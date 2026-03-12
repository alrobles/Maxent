#' @useDynLib maxentcpp, .registration = TRUE
#' @importFrom Rcpp sourceCpp
NULL

#' Create a MaxEnt Sample Object
#'
#' Creates a sample point representing a species occurrence location.
#'
#' @param lon Longitude coordinate
#' @param lat Latitude coordinate
#' @param name Sample identifier/name
#' @param dim Optional GridDimension object to calculate row/col indices
#' @return Sample object (external pointer)
#' @export
#' @examples
#' \dontrun{
#' sample <- maxent_sample(lon = -118.5, lat = 36.5, name = "site1")
#' }
maxent_sample <- function(lon, lat, name = "", dim = NULL) {
    if (is.null(dim)) {
        # Create sample with dummy row/col if no dimension provided
        create_sample(0, 0, 0, lat, lon, name)
    } else {
        rc <- coords_to_rowcol(dim, lon, lat)
        create_sample(0, rc[1], rc[2], lat, lon, name)
    }
}

#' Create a Grid Dimension Object
#'
#' Defines the spatial extent and resolution of a raster grid.
#'
#' @param nrows Number of rows
#' @param ncols Number of columns
#' @param xll X coordinate of lower-left corner
#' @param yll Y coordinate of lower-left corner
#' @param cellsize Cell size (square cells)
#' @return GridDimension object (external pointer)
#' @export
#' @examples
#' \dontrun{
#' dim <- maxent_dimension(
#'   nrows = 100, ncols = 100,
#'   xll = -120.0, yll = 35.0,
#'   cellsize = 0.1
#' )
#' }
maxent_dimension <- function(nrows, ncols, xll, yll, cellsize) {
    create_grid_dimension(nrows, ncols, xll, yll, cellsize)
}

#' Create a Grid Object
#'
#' Creates a raster grid for storing environmental variable data.
#'
#' @param dim GridDimension object defining spatial extent
#' @param name Grid layer name
#' @param nodata_value Value representing missing data (default: -9999)
#' @return Grid object (external pointer)
#' @export
#' @examples
#' \dontrun{
#' dim <- maxent_dimension(100, 100, -120, 35, 0.1)
#' grid <- maxent_grid(dim, "temperature")
#' }
maxent_grid <- function(dim, name = "", nodata_value = -9999) {
    create_grid_float(dim, name, nodata_value)
}

#' Get Grid Information
#'
#' Retrieves properties of a grid object.
#'
#' @param grid Grid object
#' @return List with grid properties
#' @export
grid_info <- function(grid) {
    # Get dimension info if available
    tryCatch({
        list(
            name = "grid",  # Would need to add getter for this
            rows = attr(grid, "rows"),
            cols = attr(grid, "cols")
        )
    }, error = function(e) {
        list(error = "Could not retrieve grid info")
    })
}

#' Convert Grid to Raster Matrix
#'
#' Extracts grid data as an R matrix.
#'
#' @param grid Grid object
#' @return Numeric matrix
#' @export
as_matrix <- function(grid) {
    grid_to_matrix(grid)
}

#' Set Grid from Matrix
#'
#' Populates grid with values from an R matrix.
#'
#' @param grid Grid object
#' @param mat Numeric matrix of values
#' @export
set_grid_matrix <- function(grid, mat) {
    grid_from_matrix(grid, mat)
    invisible(grid)
}

#' Print Sample Information
#'
#' @param x Sample object
#' @param ... Additional arguments (ignored)
#' @export
print.maxent_sample <- function(x, ...) {
    info <- get_sample_info(x)
    cat("MaxEnt Sample\n")
    cat(sprintf("  Name: %s\n", info$name))
    cat(sprintf("  Location: (%.4f, %.4f)\n", info$lon, info$lat))
    cat(sprintf("  Grid indices: [%d, %d]\n", info$row, info$col))
    invisible(x)
}
