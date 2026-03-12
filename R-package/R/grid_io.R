#' Read an ESRI ASCII Grid File
#'
#' Reads a \code{.asc} raster file into a GridFloat object.
#'
#' @param filename  Character: path to the \code{.asc} file.
#' @return External pointer to a GridFloat C++ object.
#' @export
#' @examples
#' \dontrun{
#' g <- maxent_read_asc("bio1.asc")
#' info <- maxent_grid_info(g)
#' print(info)
#' }
maxent_read_asc <- function(filename) {
    grid_read_asc(as.character(filename))
}

#' Read a Grid File (Auto-Detect Format)
#'
#' Reads a raster grid file. The format is detected from the file extension.
#' Currently supports \code{.asc} (ESRI ASCII Grid).
#'
#' @param filename  Character: path to the grid file.
#' @return External pointer to a GridFloat C++ object.
#' @export
maxent_read_grid <- function(filename) {
    grid_read_file(as.character(filename))
}

#' Write a Grid to ESRI ASCII Format
#'
#' Writes a GridFloat to a \code{.asc} file.
#'
#' @param grid       External pointer to a GridFloat object.
#' @param filename   Character: output file path.
#' @param scientific Logical: use scientific notation for floating-point
#'   values (default \code{TRUE}).
#' @export
#' @examples
#' \dontrun{
#' maxent_write_asc(g, "output.asc")
#' }
maxent_write_asc <- function(grid, filename, scientific = TRUE) {
    grid_write_asc(grid, as.character(filename), as.logical(scientific))
    invisible(filename)
}

#' Get Grid Information
#'
#' Returns metadata about a GridFloat object read from a file.
#'
#' @param grid  External pointer to a GridFloat object.
#' @return Named list with: \code{nrows}, \code{ncols}, \code{xll},
#'   \code{yll}, \code{cellsize}, \code{nodata}, \code{name},
#'   \code{count_data}.
#' @export
maxent_grid_info <- function(grid) {
    grid_float_info(grid)
}

#' Convert Grid to R Matrix
#'
#' Extracts the data from a GridFloat as an R numeric matrix.
#' NODATA cells are converted to \code{NA}.
#'
#' @param grid  External pointer to a GridFloat object.
#' @return Numeric matrix (\code{nrows} \eqn{\times} \code{ncols}).
#' @export
maxent_grid_to_matrix <- function(grid) {
    grid_float_to_matrix(grid)
}

#' Create a Grid from an R Matrix
#'
#' Builds a GridFloat from a numeric matrix and spatial parameters.
#' \code{NA} values in the matrix are stored as the \code{nodata} value.
#'
#' @param mat       Numeric matrix.
#' @param xll       X coordinate of lower-left corner.
#' @param yll       Y coordinate of lower-left corner.
#' @param cellsize  Cell size.
#' @param nodata    NODATA sentinel value (default \code{-9999}).
#' @param name      Grid name (default \code{""}).
#' @return External pointer to a GridFloat C++ object.
#' @export
maxent_grid_from_matrix <- function(mat, xll, yll, cellsize,
                                    nodata = -9999, name = "") {
    grid_float_from_matrix(as.matrix(mat),
                           as.double(xll), as.double(yll),
                           as.double(cellsize), as.double(nodata),
                           as.character(name))
}

#' Open a CSV File for Reading
#'
#' Opens a CSV file and reads column headers.
#'
#' @param filename    Character: path to the CSV file.
#' @param has_header  Logical: first line is header (default \code{TRUE}).
#' @return External pointer to a CsvReader C++ object.
#' @export
maxent_csv_open <- function(filename, has_header = TRUE) {
    csv_open(as.character(filename), as.logical(has_header))
}

#' Get CSV Column Headers
#'
#' @param reader  External pointer to a CsvReader object.
#' @return Character vector of column names.
#' @export
maxent_csv_headers <- function(reader) {
    csv_headers(reader)
}

#' Read the Next CSV Record
#'
#' @param reader  External pointer to a CsvReader object.
#' @return Character vector of field values, or \code{NULL} on EOF.
#' @export
maxent_csv_next <- function(reader) {
    csv_next_record(reader)
}

#' Read an Entire Column as Doubles
#'
#' Reads from the current file position to EOF.
#'
#' @param reader  External pointer to a CsvReader object.
#' @param field   Character: column name.
#' @return Numeric vector.
#' @export
maxent_csv_read_column <- function(reader, field) {
    csv_read_double_column(reader, as.character(field))
}

#' Close a CSV Reader
#'
#' @param reader  External pointer to a CsvReader object.
#' @export
maxent_csv_close <- function(reader) {
    csv_close(reader)
    invisible(NULL)
}

#' Create a Layer Metadata Object
#'
#' @param name      Character: layer name.
#' @param type      Character: layer type. One of \code{"Continuous"},
#'   \code{"Categorical"}, \code{"Bias"}, \code{"Mask"},
#'   \code{"Probability"}, \code{"Cumulative"}, \code{"DebiasAvg"},
#'   or \code{"Unknown"}.
#' @return External pointer to a Layer C++ object.
#' @export
maxent_layer <- function(name, type = "Continuous") {
    create_layer(as.character(name), as.character(type))
}

#' Get Layer Metadata
#'
#' @param layer  External pointer to a Layer object.
#' @return Named list with \code{name} and \code{type}.
#' @export
maxent_layer_info <- function(layer) {
    get_layer_info(layer)
}

#' Extract Layer Name from a File Path
#'
#' Strips directory prefix and extension.
#'
#' @param path  Character: file path.
#' @return Character: layer name (e.g., \code{"/data/bio1.asc"} → \code{"bio1"}).
#' @export
maxent_layer_name <- function(path) {
    layer_name_from_path(as.character(path))
}
