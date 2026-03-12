#include <Rcpp.h>

// Enable C++17
// [[Rcpp::plugins(cpp17)]]

// Link to Eigen
// [[Rcpp::depends(RcppEigen)]]

#include "../../../cpp/include/maxent/feature.hpp"
#include "../../../cpp/include/maxent/featured_space.hpp"

using namespace Rcpp;
using namespace maxent;

// -------------------------------------------------------------------------
// Helper: extract a vector of Feature* from a list of XPtr<Feature>
// -------------------------------------------------------------------------
static std::vector<std::shared_ptr<Feature>> extract_features(List feature_ptrs) {
    std::vector<std::shared_ptr<Feature>> features;
    features.reserve(feature_ptrs.size());
    for (int i = 0; i < feature_ptrs.size(); ++i) {
        XPtr<Feature> xp(feature_ptrs[i]);
        // Wrap the raw pointer in a non-owning shared_ptr
        // (the XPtr still manages lifetime from R)
        features.push_back(std::shared_ptr<Feature>(xp.get(), [](Feature*) {}));
    }
    return features;
}

//' Create a FeaturedSpace object
//'
//' Constructs a MaxEnt FeaturedSpace from background point count,
//' occurrence sample indices, and a list of Feature objects.
//'
//' @param num_points     Integer: number of background points.
//' @param sample_indices Integer vector: 0-based indices of occurrence samples
//'   in the background array.
//' @param feature_ptrs   List of external pointers to Feature objects
//'   (from \code{create_linear_feature()} etc.).
//' @return External pointer to a FeaturedSpace object.
//' @export
// [[Rcpp::export]]
SEXP maxent_featured_space_create(int num_points,
                                  IntegerVector sample_indices,
                                  List feature_ptrs) {
    std::vector<int> idx(sample_indices.begin(), sample_indices.end());
    auto features = extract_features(feature_ptrs);

    FeaturedSpace* fs = new FeaturedSpace(num_points, idx, features);
    XPtr<FeaturedSpace> ptr(fs, true);
    return ptr;
}

//' Train a FeaturedSpace model
//'
//' Runs the sequential coordinate-ascent MaxEnt optimization.
//'
//' @param fs_ptr            External pointer to a FeaturedSpace object.
//' @param max_iter          Maximum number of training iterations (default 500).
//' @param convergence       Convergence threshold (default 1e-5).
//' @param beta_multiplier   Regularization multiplier (default 1.0).
//' @param min_deviation     Minimum sample deviation floor (default 0.001).
//' @return Named list with elements: \code{loss}, \code{entropy},
//'   \code{iterations}, \code{converged}, \code{lambdas}.
//' @export
// [[Rcpp::export]]
List maxent_train(SEXP fs_ptr,
                  int    max_iter        = 500,
                  double convergence     = 1e-5,
                  double beta_multiplier = 1.0,
                  double min_deviation   = 0.001) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    TrainResult r = fs->train(max_iter, convergence, beta_multiplier, min_deviation);

    return List::create(
        Named("loss")       = r.loss,
        Named("entropy")    = r.entropy,
        Named("iterations") = r.iterations,
        Named("converged")  = r.converged,
        Named("lambdas")    = NumericVector(r.lambdas.begin(), r.lambdas.end())
    );
}

//' Predict with a trained FeaturedSpace model
//'
//' Computes raw Gibbs distribution scores for new environmental data.
//'
//' @param fs_ptr    External pointer to a trained FeaturedSpace object.
//' @param new_data  Numeric matrix with one row per new point and one column
//'   per feature (values must be pre-evaluated, i.e., the feature
//'   transformation already applied).
//' @return Numeric vector of raw scores (unnormalized).
//' @export
// [[Rcpp::export]]
NumericVector maxent_predict(SEXP fs_ptr, NumericMatrix new_data) {
    XPtr<FeaturedSpace> fs(fs_ptr);

    int n_pts = new_data.nrow();
    int n_feat = new_data.ncol();

    std::vector<std::vector<double>> mat(n_pts, std::vector<double>(n_feat));
    for (int i = 0; i < n_pts; ++i) {
        for (int j = 0; j < n_feat; ++j) {
            mat[i][j] = new_data(i, j);
        }
    }

    auto scores = fs->predict(mat);
    return NumericVector(scores.begin(), scores.end());
}

//' Get current distribution weights from a FeaturedSpace
//'
//' @param fs_ptr External pointer to a FeaturedSpace object.
//' @return Numeric vector of normalized weights (sums to 1).
//' @export
// [[Rcpp::export]]
NumericVector maxent_get_weights(SEXP fs_ptr) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    auto w = fs->get_weights();
    return NumericVector(w.begin(), w.end());
}

//' Get entropy of a FeaturedSpace distribution
//'
//' @param fs_ptr External pointer to a FeaturedSpace object.
//' @return Shannon entropy (non-negative scalar).
//' @export
// [[Rcpp::export]]
double maxent_get_entropy(SEXP fs_ptr) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    return fs->get_entropy();
}

//' Get current loss of a FeaturedSpace
//'
//' @param fs_ptr External pointer to a FeaturedSpace object.
//' @return Scalar loss value (negative log-likelihood).
//' @export
// [[Rcpp::export]]
double maxent_get_loss(SEXP fs_ptr) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    return fs->get_loss();
}

//' Write feature lambdas to a file
//'
//' Saves the trained model coefficients in CSV format compatible with
//' the original Java Maxent .lambdas file format.
//'
//' @param fs_ptr   External pointer to a trained FeaturedSpace object.
//' @param filename Character: path to the output file.
//' @export
// [[Rcpp::export]]
void maxent_write_lambdas(SEXP fs_ptr, std::string filename) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    fs->write_lambdas(filename);
}

//' Read feature lambdas from a file
//'
//' Restores model coefficients from a .lambdas file.  The FeaturedSpace
//' must have been created with the same features (same names and order).
//'
//' @param fs_ptr   External pointer to a FeaturedSpace object.
//' @param filename Character: path to the lambdas file.
//' @export
// [[Rcpp::export]]
void maxent_read_lambdas(SEXP fs_ptr, std::string filename) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    fs->read_lambdas(filename);
}

//' Set sample expectations for a FeaturedSpace
//'
//' Computes sample expectations and regularization deviations for each feature.
//' Called automatically by \code{maxent_train()}, but exposed for advanced use.
//'
//' @param fs_ptr          External pointer to a FeaturedSpace object.
//' @param beta_multiplier Regularization multiplier (default 1.0).
//' @param min_deviation   Minimum sample deviation (default 0.001).
//' @export
// [[Rcpp::export]]
void maxent_set_sample_expectations(SEXP fs_ptr,
                                    double beta_multiplier = 1.0,
                                    double min_deviation   = 0.001) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    fs->set_sample_expectations(beta_multiplier, min_deviation);
}

//' Get FeaturedSpace metadata
//'
//' @param fs_ptr External pointer to a FeaturedSpace object.
//' @return Named list with num_points, num_samples, num_features,
//'   density_normalizer, linear_predictor_normalizer.
//' @export
// [[Rcpp::export]]
List maxent_featured_space_info(SEXP fs_ptr) {
    XPtr<FeaturedSpace> fs(fs_ptr);
    return List::create(
        Named("num_points")   = fs->num_points(),
        Named("num_samples")  = fs->num_samples(),
        Named("num_features") = fs->num_features(),
        Named("density_normalizer") = fs->get_density_normalizer(),
        Named("linear_predictor_normalizer") = fs->get_linear_predictor_normalizer()
    );
}
