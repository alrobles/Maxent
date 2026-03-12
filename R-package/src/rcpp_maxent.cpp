#include <Rcpp.h>
#include <RcppEigen.h>

// Enable C++17
// [[Rcpp::plugins(cpp17)]]

// Link to Eigen
// [[Rcpp::depends(RcppEigen)]]

#include "../../../cpp/include/maxent/grid_dimension.hpp"
#include "../../../cpp/include/maxent/sample.hpp"
#include "../../../cpp/include/maxent/grid.hpp"

using namespace Rcpp;
using namespace maxent;

//' Create a GridDimension object
//'
//' Creates a grid dimension specifying the spatial extent and resolution
//' 
//' @param nrows Number of rows
//' @param ncols Number of columns
//' @param xll X coordinate of lower-left corner
//' @param yll Y coordinate of lower-left corner
//' @param cellsize Cell size (assumed square cells)
//' @return External pointer to GridDimension object
//' @export
// [[Rcpp::export]]
SEXP create_grid_dimension(int nrows, int ncols, double xll, double yll, double cellsize) {
    GridDimension* dim = new GridDimension(nrows, ncols, xll, yll, cellsize);
    XPtr<GridDimension> ptr(dim, true);
    return ptr;
}

//' Get grid dimension properties
//'
//' @param dim_ptr External pointer to GridDimension
//' @return List with dimension properties
//' @export
// [[Rcpp::export]]
List get_grid_dimension_info(SEXP dim_ptr) {
    XPtr<GridDimension> dim(dim_ptr);
    
    return List::create(
        Named("nrows") = dim->nrows,
        Named("ncols") = dim->ncols,
        Named("xll") = dim->xll,
        Named("yll") = dim->yll,
        Named("cellsize") = dim->cellsize,
        Named("size") = dim->size()
    );
}

//' Convert geographic coordinates to row/col
//'
//' @param dim_ptr External pointer to GridDimension
//' @param lon Longitude
//' @param lat Latitude
//' @return Integer vector with row and column indices
//' @export
// [[Rcpp::export]]
IntegerVector coords_to_rowcol(SEXP dim_ptr, double lon, double lat) {
    XPtr<GridDimension> dim(dim_ptr);
    auto rc = dim->toRowCol(lon, lat);
    return IntegerVector::create(rc[0], rc[1]);
}

//' Create a Sample object
//'
//' Creates a species occurrence sample point
//'
//' @param point Point identifier
//' @param row Row index in grid
//' @param col Column index in grid
//' @param lat Latitude coordinate
//' @param lon Longitude coordinate
//' @param name Sample name/identifier
//' @return External pointer to Sample object
//' @export
// [[Rcpp::export]]
SEXP create_sample(int point, int row, int col, double lat, double lon, String name) {
    Sample* sample = new Sample(point, row, col, lat, lon, as<std::string>(name));
    XPtr<Sample> ptr(sample, true);
    return ptr;
}

//' Get sample properties
//'
//' @param sample_ptr External pointer to Sample
//' @return List with sample properties
//' @export
// [[Rcpp::export]]
List get_sample_info(SEXP sample_ptr) {
    XPtr<Sample> sample(sample_ptr);
    
    return List::create(
        Named("point") = sample->getPoint(),
        Named("row") = sample->getRow(),
        Named("col") = sample->getCol(),
        Named("lat") = sample->getLat(),
        Named("lon") = sample->getLon(),
        Named("name") = sample->getName()
    );
}

//' Set feature value on a sample
//'
//' @param sample_ptr External pointer to Sample
//' @param feature_name Name of the feature
//' @param value Feature value
//' @export
// [[Rcpp::export]]
void sample_set_feature(SEXP sample_ptr, String feature_name, double value) {
    XPtr<Sample> sample(sample_ptr);
    sample->setFeature(as<std::string>(feature_name), value);
}

//' Get feature value from a sample
//'
//' @param sample_ptr External pointer to Sample
//' @param feature_name Name of the feature
//' @param default_val Default value if feature not found
//' @return Feature value
//' @export
// [[Rcpp::export]]
double sample_get_feature(SEXP sample_ptr, String feature_name, double default_val = 0.0) {
    XPtr<Sample> sample(sample_ptr);
    return sample->getFeature(as<std::string>(feature_name), default_val);
}

//' Create a float Grid object
//'
//' Creates a raster grid for environmental variables
//'
//' @param dim_ptr External pointer to GridDimension
//' @param name Grid layer name
//' @param nodata_value Value representing missing data
//' @return External pointer to Grid<float> object
//' @export
// [[Rcpp::export]]
SEXP create_grid_float(SEXP dim_ptr, String name, double nodata_value = -9999.0) {
    XPtr<GridDimension> dim(dim_ptr);
    GridFloat* grid = new GridFloat(*dim, as<std::string>(name), static_cast<float>(nodata_value));
    XPtr<GridFloat> ptr(grid, true);
    return ptr;
}

//' Get grid value
//'
//' @param grid_ptr External pointer to Grid
//' @param row Row index
//' @param col Column index
//' @return Grid value
//' @export
// [[Rcpp::export]]
double grid_get_value(SEXP grid_ptr, int row, int col) {
    XPtr<GridFloat> grid(grid_ptr);
    return static_cast<double>(grid->getValue(row, col));
}

//' Set grid value
//'
//' @param grid_ptr External pointer to Grid
//' @param row Row index
//' @param col Column index
//' @param value Value to set
//' @export
// [[Rcpp::export]]
void grid_set_value(SEXP grid_ptr, int row, int col, double value) {
    XPtr<GridFloat> grid(grid_ptr);
    grid->setValue(row, col, static_cast<float>(value));
}

//' Check if grid cell has data
//'
//' @param grid_ptr External pointer to Grid
//' @param row Row index
//' @param col Column index
//' @return TRUE if cell has valid data
//' @export
// [[Rcpp::export]]
bool grid_has_data(SEXP grid_ptr, int row, int col) {
    XPtr<GridFloat> grid(grid_ptr);
    return grid->hasData(row, col);
}

//' Get grid as matrix
//'
//' @param grid_ptr External pointer to Grid
//' @return Numeric matrix of grid values
//' @export
// [[Rcpp::export]]
NumericMatrix grid_to_matrix(SEXP grid_ptr) {
    XPtr<GridFloat> grid(grid_ptr);
    int nrows = grid->getRows();
    int ncols = grid->getCols();
    
    NumericMatrix mat(nrows, ncols);
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            mat(i, j) = static_cast<double>(grid->getValue(i, j));
        }
    }
    return mat;
}

//' Set grid from matrix
//'
//' @param grid_ptr External pointer to Grid
//' @param mat Numeric matrix of values
//' @export
// [[Rcpp::export]]
void grid_from_matrix(SEXP grid_ptr, NumericMatrix mat) {
    XPtr<GridFloat> grid(grid_ptr);
    int nrows = mat.nrow();
    int ncols = mat.ncol();
    
    if (nrows != grid->getRows() || ncols != grid->getCols()) {
        stop("Matrix dimensions don't match grid dimensions");
    }
    
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            grid->setValue(i, j, static_cast<float>(mat(i, j)));
        }
    }
}
