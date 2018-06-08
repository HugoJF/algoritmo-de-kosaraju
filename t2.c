#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/******************
 * CONSOLE COLORS *
 ******************/


#define RED             "\x1b[31m"
#define GREEN           "\x1b[32m"
#define YELLOW          "\x1b[33m"
#define BLUE            "\x1b[34m"
#define MAGENTA         "\x1b[35m"
#define CYAN            "\x1b[36m"
#define RESET           "\x1b[0m"

#define BRIGHT_RED      "\x1b[91m"
#define BRIGHT_GREEN    "\x1b[92m"
#define BRIGHT_YELLOW   "\x1b[93m"
#define BRIGHT_BLUE     "\x1b[94m"
#define BRIGHT_MAGENTA  "\x1b[95m"
#define BRIGHT_CYAN     "\x1b[96m"

#define BOLD            "\x1b[1m"
#define BOLD_OFF        "\x1b[21m"
#define FAINT           "\x1b[2m"
#define FAINT_OFF       "\x1b[22m"
#define ITALIC          "\x1b[3m"
#define ITALIC_OFF      "\x1b[23m"
#define UNDERLINE       "\x1b[4m"
#define UNDERLINE_OFF   "\x1b[24m"
#define REVERSE         "\x1b[7m"
#define REVERSE_OFF     "\x1b[27m"

/*
 * FIXME: Update console output colors
 *
 * BLUE:      values
 * WHITE:     labels
 * GREEN:     good-result
 * RED:       bad-result
 * MAGENTA:   actions
 * YELLOW:    information
 *
 * BOLD:      header
 * UNDERLINE: tree-root label
 * REVERSE:   critical
 */

// Verificar se existe outra exigencia

// BUG: Atualizar peso das arestas do novo vertice, deve levar em consideracao o caminho forward e reverse: https://i.imgur.com/vMrwMOY.png

// FIXME: Unify graph printing code

// FIXME: Improve verbose debugging

/*************************
 * STRUCTURE DEFINITIONS *
 *************************/


typedef struct vector_ {
    int size;
    int count;
    void **data;
} vector;

typedef struct edge_ {
    int destination;
    float weight;
} edge;

typedef struct vertex_ {
    int id;
    float latitude;
    float longitude;
    int edge_count;
    edge *edges;
    int origins_count;
    struct vertex_ **origins;
} vertex;


/********************
 * GLOBAL VARIABLES *
 ********************/

// File paths
char *input_path;
char *compressed_graph_path;
char *compressed_paths_path;

// File handles
FILE *input_file;
FILE *compressed_output_file;
FILE *compresed_paths_file;

// FIXME: deprecated variable
int total_vertex_count;

// Compression stats
int wiped_vertices = 0;
int wiped_edges = 0;

// The main graph
vector *vertices;

// List of paths found
vector *one_way_paths_found;
vector *two_way_paths_found;


/********************
 * VECTOR FUNCTIONS *
 ********************/


/***
 * Initializes vector `v`
 *
 * @param v
 */
void vector_init(vector *v) {
    v->data = NULL;
    v->size = 0;
    v->count = 0;
}

/***
 * Adds new pointer to vector
 *
 * @param v - vector to hold data
 * @param e - pointer to be added
 */
void vector_add(vector *v, void *e) {
    if (v->size == 0) {
        v->size = 10;
        v->data = malloc(sizeof(void *) * v->size);
        memset(v->data, '\0', sizeof(void) * v->size);
    }

    // Reallocate when last slot is used
    if (v->size == v->count) {
        v->size *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->size);
    }

    v->data[v->count] = e;
    v->count++;
}

/***
 * Set value in specific position inside vector
 *
 * @param v - vector to modify
 * @param index - index of pointer to replace
 * @param e  - replacement pointer
 */
void vector_set(vector *v, int index, void *e) {
    if (index >= v->count) {
        return;
    }

    v->data[index] = e;
}

/***
 * Returns pointer from `index`
 *
 * @param v - vector to access
 * @param index - index of pointer
 * @return pointer from index `index` inside `v`
 */
void *vector_get(vector *v, int index) {
    if (index >= v->count) {
        return NULL;
    }

    return v->data[index];
}

/**
 * Removes pointer from index `index`
 *
 * @param v - vector to modify
 * @param index - position of pointer to delete
 */
void vector_delete(vector *v, int index) {
    if (index >= v->count) {
        return;
    }

    v->data[index] = NULL;

    int i, j;
    void **newarr = (void **) malloc(sizeof(void *) * v->count * 2);
    for (i = 0, j = 0; i < v->count; i++) {
        if (v->data[i] != NULL) {
            newarr[j] = v->data[i];
            j++;
        }
    }

    free(v->data);

    v->data = newarr;
    v->count--;
}

/**
 * Free memory allocated to vecto data
 *
 * @param v - vector to free
 */
void vector_free(vector *v) {
    free(v->data);
}


/******************
 * PATH FUNCTIONS *
 ******************/


/**
 * Checks if vertex with ID `id` is in `path`
 *
 * @param id - vertex ID to be checked
 * @param path - list of vertices forming a path
 * @return if `id` is in `path`
 */
bool in_path(int id, vector *path) {
    vertex *v;

    for (int i = 0; i < path->count; ++i) {
        v = vector_get(path, i);

        if (v->id == id) {
            return true;
        }
    }

    return false;
}

/**
 * Checks if vertex with ID `id` is in `path`
 *
 * @param needle - vertex to be checked
 * @param path - list of vertices forming a path
 * @return if `needle` is in `path`
 */
bool in_path_vert(vertex *needle, vector *path) {
    return in_path(needle->id, path);
}


/****************
 * INDEX AND ID *
 ***************/


/***
 * Convert ID to array index
 *
 * @param id - id to be converted
 * @return array index associated with `id`
 */
int id_to_index(int id) {
    return id - 1;
}


/**
 * Convert array index to vertex ID
 *
 * @param index - index to be converted
 * @return id associated with array index `index`
 */
int index_to_id(int index) {
    return index + 1;
}


/********************
 * VERTEX FUNCTIONS *
 ********************/


/***
 * Gets vertex with ID `id`
 *
 * @param id - ID to be returned
 * @return vertex with ID `id`
 */
vertex *get_vertex_by_id(int id) {
    return vector_get(vertices, id_to_index(id));
}


/***
 * Check and returns `edge` from `origin` to `destination`
 *
 * @param origin - vertex ID
 * @param destination - vertex ID
 * @return edge from `origin` to `destination`
 */
edge *vertex_has_destination(int origin, int destination) {
    vertex *v = get_vertex_by_id(origin);

    for (int i = 0; i < v->edge_count; i++) {
        v = get_vertex_by_id(origin);
        if (v->edges[i].destination == destination) {
            return &v->edges[i];
        }
    }

    return NULL;
}

/***
 * Check and returns `edge` from `origin` to `destination`
 *
 * @param origin - vertex ID
 * @param destination - vertex ID
 * @return edge from `origin` to `destination`
 */
edge *vertex_has_destination_vert(vertex *origin, vertex *destination) {
    return vertex_has_destination(origin->id, destination->id);
}

/**
 * Check if vertex with ID `v` has another vertex, with ID `origin`, pointing to it.
 *
 * @param v - vertex ID to check
 * @param origin - origin to be checked
 * @return if `origin` points to `v`
 */
bool vertex_has_origin(int v, int origin) {
    vertex *v1 = get_vertex_by_id(v);

    for (int i = 0; i < v1->origins_count; ++i) {
        if (v1->origins[i]->id == origin) {
            return true;
        }
    }

    return false;
}

/**
 * Check if vertex `v` has vertex `origin`, pointing to it.
 *
 * @param v - vertex to check
 * @param origin - origin to be checked
 * @return if `origin` points to `v`
 */
bool vertex_has_origin_vert(vertex *v, vertex *origin) {
    return vertex_has_origin(v->id, origin->id);
}


/******************
 * PATH FUNCTIONS *
 ******************/


/**
 * Check if paths are equal
 *
 * @param path1 - path a
 * @param path2 - path b
 * @return if path `a` == `b`
 */
bool path_equals(vector *path1, vector *path2) {
    vertex *v;
    for (int i = 0; i < path1->count; ++i) {
        v = vector_get(path1, i);
        if (!in_path(v->id, path2)) {
            return false;
        }
    }

    return true;
}

/**
 * If a list of paths, contains a path
 *
 * @param path1 - path to be checked
 * @param list - list of paths
 * @return if `path1` is in `list`
 */
bool path_exists(vector *path1, vector *list) {
    vector *path2;
    for (int i = 0; i < list->count; ++i) {
        path2 = vector_get(list, i);
        if (path_equals(path1, path2)) {
            return true;
        }

    }

    return false;
}


/*******************
 * PATH VALIDATION *
 *******************/


bool validate_one_way_path_length(vector *path) {
    return path->count >= 4;
}

bool validate_two_way_path_length(vector *path) {
    return path->count >= 4;
}

bool validate_one_way_path_head(vector *path) {
    vertex *v1, *v2;

    v1 = vector_get(path, 0);
    v2 = vector_get(path, 1);

    // Avoid isolated cycles and check if one way path condition is valid
    return (v1->edge_count > 1 || v1->origins_count > 1) &&
           vertex_has_destination_vert(v1, v2) != NULL;
}

bool validate_two_way_path_head(vector *path) {
    vertex *v1, *v2;

    v1 = vector_get(path, 0);
    v2 = vector_get(path, 1);

    // Avoid isolated cycles and check if one way path condition is valid
    return (v1->edge_count > 2 || v1->origins_count > 2) &&
           vertex_has_destination_vert(v1, v2) != NULL &&
           vertex_has_origin_vert(v1, v2);
}

bool validate_one_way_path_internals(vector *path) {
    for (int i = 1; i < path->count - 1; ++i) {
        if (!vertex_has_origin_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i - 1)) ||
            vertex_has_destination_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i + 1)) == NULL) {
            return false;
        }
    }
    return true;
}

bool validate_two_way_path_internals(vector *path) {
    for (int i = 1; i < path->count - 1; ++i) {
        if (
                !vertex_has_origin_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i - 1)) ||
                !vertex_has_origin_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i + 1)) ||
                vertex_has_destination_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i + 1)) == NULL ||
                vertex_has_destination_vert((vertex *) vector_get(path, i), (vertex *) vector_get(path, i - 1)) == NULL
                ) {
            return false;
        }
    }
    return true;
}

bool validate_one_way_path_tail(vector *path) {
    vertex *v1, *v2;

    v1 = vector_get(path, path->count - 1);
    v2 = vector_get(path, path->count - 2);

    // Avoid isolated cycles and check if one way path condition is valid
    return (v1->edge_count > 1 || v1->origins_count > 1) &&
           vertex_has_origin_vert(v1, v2);
}

bool validate_two_way_path_tail(vector *path) {
    vertex *v1, *v2;

    v1 = vector_get(path, path->count - 1);
    v2 = vector_get(path, path->count - 2);

    // Avoid isolated cycles and check if one way path condition is valid
    return (v1->edge_count > 2 || v1->origins_count > 2) &&
           vertex_has_origin_vert(v1, v2) &&
           vertex_has_destination_vert(v1, v2) != NULL;
}

bool validate_one_way_path(vector *path) {
    return validate_one_way_path_length(path) &&
           validate_one_way_path_head(path) &&
           validate_one_way_path_internals(path) &&
           validate_one_way_path_tail(path);
}

bool validate_two_way_path(vector *path) {
    return validate_two_way_path_length(path) &&
           validate_two_way_path_head(path) &&
           validate_two_way_path_internals(path) &&
           validate_two_way_path_tail(path);
}

/**
 * Prints user-friedly list of edges
 *
 * @param edges
 * @param edge_count
 */
void print_edges(edge *edges, int edge_count) {
    for (int i = 0; i < edge_count; ++i) {
        printf("\tDestination: %d\n"
               "\tWeight: %f\n", edges[i].destination, edges[i].weight);
    }
}


/***
 * Prints user-friendly main graph to screen
 */
void print_graph() {
    vertex *vert;
    printf(BLUE);
    for (int i = 0; i < vertices->count; ++i) {
        vert = vector_get(vertices, i);

        printf("SERIALIZING VERTEX [ %d / %d ]\n"
               "ID :        %d\n"
               "Latitude:   %f\n"
               "Longitude:  %f\n"
               "Edge Count: %d\n", vert->id, vertices->count, vert->id, vert->latitude, vert->longitude,
               vert->edge_count);

        print_edges(vert->edges, vert->edge_count);
    }
    printf(RESET);
}


/****************
 * STDOUT UTILS *
 ****************/


/***
 * Prints character `c`, `amount` times.
 *
 * @param c - character to be repeated
 * @param amount - amount of times to repeated
 */
void repeat_char(char *c, int amount) {
    for (int i = 0; i < amount; ++i) {
        printf("%s", c);
    }
}

/***
 * Prints big 'billboard' on screen
 *
 * @param text - text in the middle of the billboard
 */
void spam(char *text) {
    int tab_size = 8;

    printf(BOLD "\n");
    repeat_char("#", strlen(text) + tab_size * 2);
    printf("\n#");
    repeat_char(" ", tab_size - 1);
    printf(text);
    repeat_char(" ", tab_size - 1);
    printf("#\n");
    repeat_char("#", strlen(text) + tab_size * 2);
    printf(RESET "\n\n");
}

/***
 * Prints user-friendly path vertex list
 *
 * @param path
 */
void print_path(vector *path) {
    vertex *v;

    for (int i = 0; i < path->count; ++i) {
        v = (vector_get(path, i));
        if (i == 0) {
            printf("< ");
        }

        printf("%d", v->id);

        if (i == path->count - 1) {
            printf(" >\n");
        } else {
            printf(", ");
        }
    }
}


/********************
 * GRAPH PROCESSING *
 ********************/


/***
 * Finds compressible path, starting from a base vertex
 *
 * @param base - vertex used as a base
 * @param path - resulting path found
 * @param degree - degree of path to be searched
 * @return - size of found path
 */
int find_compressible_path(vertex *base, vector *path, int degree) {
    int size = 1; // base is added to path as step 1
    vertex *origin = base;
    vertex *old_origin = NULL;

    vertex *destination = base;
    vertex *old_destination = NULL;

    spam("Finding compressible path");
    printf(MAGENTA);
    printf("Running find_compressible_path from vertex %d\n", base->id);
    printf(RESET);

    // Temporary placeholder for back-path
    vector back_path;
    // FIXME: POINTERS POINTERS POINTERS
    // Init placeholder
    vector_init(&back_path);

    // Moves origin back until path cannot be increased
    while (origin->origins_count == degree && origin->edge_count == degree) {

        if (!in_path_vert(origin->origins[0], &back_path)) {
            origin = origin->origins[0];
        } else if (!in_path_vert(origin->origins[1], &back_path) && degree == 2) {
            origin = origin->origins[1];
        } else {
            break;
        }

        vector_add(&back_path, origin);

        printf(MAGENTA "Moving origin to vertex %d\n" RESET, origin->id);

        // Avoids looping
        if (origin == base) {
            break;
        }

        size++;
    }

    // Reverse back-path to keep everything ordered
    for (int i = back_path.count - 1; i >= 0; --i) {
        vector_add(path, vector_get(&back_path, i));
    }

    // Add the initial vertex
    vector_add(path, base);

    // Continue expanding forward while path can be increased
    while (destination->edge_count == degree && destination->origins_count == degree) {

        if (!in_path(destination->edges[0].destination, path)) {
            destination = get_vertex_by_id(destination->edges[0].destination);
        } else if (!in_path(destination->edges[1].destination, path) && degree == 2) {
            destination = get_vertex_by_id(destination->edges[1].destination);
        } else {
            break;
        }

        vector_add(path, destination);

        printf(MAGENTA "Moving destination to vertex %d\n" RESET, destination->id);

        // Avoids infinite looping
        if (destination == base) {
            break;
        }

        size++;
    }

    return size;
}

/***
 * Sums every edge in the path, that points to a vertex in the path
 *
 * @param path - path to sum it's edges
 * @return sum of internal edges
 */
float calculate_internal_edge_weights(vector *path) {
    vertex *v, *v2;
    float total_weight = 0;
    for (int j = 0; j < path->count; j++) {
        v = vector_get(path, j);

        // Edge vertices (head/tail) destinations should only be summed if they point to vertices in the path
        if (j == 0 || j == path->count - 1) {
            for (int i = 0; i < v->edge_count; ++i) {
                v2 = vector_get(path, j + (j == 0 ? 1 : -1));
                if (v->edges[i].destination == v2->id) {
                    printf(MAGENTA "Adding %d to %d is in path, adding %f\n" RESET, v->id, v->edges[i].destination,
                           v->edges[i].weight);
                    total_weight += v->edges[i].weight;
                    continue;
                }
            }
        } else {
            // Internal vertices destinations should always be summed (since they always point to the path)
            for (int k = 0; k < v->edge_count; ++k) {
                total_weight += v->edges[k].weight;
            }
        }
    }

    return total_weight;
}

/**
 * Parses main graph file
 */
void parse_graph_file() {
    vertex *v;
    char parameterType;
    int total_edge_count, max_degree;

    printf("Reading file header... \n\n");

    // Read file header and prepare for vertex reading

    fscanf(input_file, "%c %d %d %d\n", &parameterType, &total_vertex_count, &total_edge_count, &max_degree);

    printf("ParameterType:      %c\n"
           "Total Vertex Count: %d\n"
           "Total Edge Count:   %d\n"
           "Max Graph Degree:   %d\n", parameterType, total_vertex_count, total_edge_count, max_degree);


    // Init vertices vector
    vertices = malloc(sizeof(vector));
    vector_init(vertices);

    int id, edge_count;
    float latitude, longitude;

    /**
     * Starts reading each line for vertex information
     */


    for (int vertex_index = 0; vertex_index < total_vertex_count; vertex_index++) {

        v = malloc(sizeof(vertex));

        printf(GREEN "\nREADING VERTEX [ %d / %d ]\n" RESET, vertex_index + 1,
               total_vertex_count);

        fscanf(input_file, "%c %d %f %f %d", &parameterType, &id, &latitude, &longitude, &edge_count);

        printf("|\tParameter Type: " BRIGHT_BLUE "%c\n" RESET
               "|\tID:             " BRIGHT_BLUE "%d\n" RESET
               "|\tLatitude:       " BRIGHT_BLUE "%f\n" RESET
               "|\tLongitude:      " BRIGHT_BLUE "%f\n" RESET
               "|\tEdge Count:     " BRIGHT_BLUE "%d\n" RESET, parameterType, id, latitude, longitude, edge_count);


        // Save vertex information to struct
        v->id = id;
        v->latitude = latitude;
        v->longitude = longitude;
        v->edge_count = edge_count;
        v->edges = malloc(sizeof(edge) * edge_count);
        v->origins_count = 0;

        vector_add(vertices, v);

        int destination;
        float weight;

        /**
         * Reads edge information for current vertex
         */
        for (int edge_index = 0; edge_index < edge_count; edge_index++) {

            fscanf(input_file, "%d %f", &destination, &weight);

            printf("|\t|\t" GREEN "READING EDGE [ %d / %d ]\n" RESET
                   "|\t|\t|\tDestination: " BRIGHT_BLUE "%d\n" RESET
                   "|\t|\t|\tWeight:      " BRIGHT_BLUE "%f\n" RESET, edge_index + 1, edge_count, destination, weight);

            v->edges[edge_index].destination = destination;
            v->edges[edge_index].weight = weight;
        }

        // Consume new line character to avoid miss-alignment
        fscanf(input_file, "\n");
        printf(GREEN "ENDING VERTEX  [ %d / %d ]\n" RESET, vertex_index + 1, total_vertex_count);
    }
}

/***
 * Performs entire graph search for compressible paths
 */
void search_compressible_paths() {

    float internal_weight = 0;

    vector *one_way_path;
    vector *two_way_path;

    one_way_paths_found = malloc(sizeof(vector));
    two_way_paths_found = malloc(sizeof(vector));

    vector_init(one_way_paths_found);
    vector_init(two_way_paths_found);

    for (int i = 0; i < vertices->count; i++) {
        one_way_path = malloc(sizeof(vector));
        two_way_path = malloc(sizeof(vector));

        vector_init(one_way_path);
        vector_init(two_way_path);

        find_compressible_path(vector_get(vertices, i), one_way_path, 1);
        find_compressible_path(vector_get(vertices, i), two_way_path, 2);

        // Check if two-way-path is valid first, since two-way-paths can be false-validated as one-way-paths
        if (validate_two_way_path(two_way_path)) {
            // Output two_way_path according to type
            if (path_exists(two_way_path, two_way_paths_found)) {
                printf(RED "Found duplicate two_way_path, ignoring for now...\n" RESET);
                vector_free(two_way_path);
                vector_init(two_way_path);
                // FIXME: Memory leak reallocate memory on path found. Clear vector on duplicate/notfound
                continue;
            } else {
                printf(GREEN "Path found is not duplicate\n" RESET);
                vector_add(two_way_paths_found, two_way_path);
            }
            printf("Debugging valid two_way_path with size %d\n", two_way_path->count);
            print_path(two_way_path);

            // Calculate and output two_way_path internal weight used for calculation
            internal_weight = calculate_internal_edge_weights(two_way_path);
            printf("Internal Segments Weight: %f \n", internal_weight);
            printf("Each new edge will have: %f \n", internal_weight / 4);

        } else if (validate_one_way_path(one_way_path)) {
            // Output two_way_path according to type
            if (path_exists(one_way_path, one_way_paths_found)) {
                printf(RED "Found duplicate one_way_path, ignoring for now...\n" RESET);
                vector_free(one_way_path);
                vector_init(one_way_path);
                // FIXME: Memory leak
                continue;
            } else {
                printf(GREEN "Path found is not duplicate\n" RESET);
                vector_add(one_way_paths_found, one_way_path);
            }
            printf("Debugging valid one_way_path with size %d\n", two_way_path->count);
            print_path(one_way_path);


            // Calculate and output two_way_path internal weight used for calculation
            internal_weight = calculate_internal_edge_weights(one_way_path);
            printf("Internal Segments Weight: %f \n", internal_weight);
            printf("Each new edge will have: %f \n", internal_weight / 2);
        } else {
            bool length = false,
                    head = false,
                    internals = false,
                    tail = false;

            length = validate_two_way_path_length(two_way_path);

            if (length) {
                head = validate_two_way_path_head(two_way_path);
            }

            if (length && head) {
                internals = validate_two_way_path_internals(two_way_path);
            }

            if (length && head && internal_weight) {
                tail = validate_two_way_path_tail(two_way_path);
            }


            printf(RED BOLD UNDERLINE "Path failed validation summary: ");
            print_path(two_way_path);
            printf(BOLD_OFF UNDERLINE_OFF RED);
            printf("|\tLength validation:   %d\n", length);
            printf("|\tHead validation:     %d\n", head);
            printf("|\tInternal validation: %d\n", internals);
            printf("|\tTail validation:     %d\n", tail);
            printf(RESET "\n");
        }
    }
}

/***
 * Pre process graph vertices to count how many other vertices point to it
 */
// FIXME: this should already be done in pre_process_origins() since code is now using vectors
void pre_process_origins_count() {
    vertex *v1, *v2;

    for (int i = 0; i < vertices->count; ++i) {
        v1 = vector_get(vertices, i);
        for (int j = 0; j < v1->edge_count; ++j) {
            v2 = vector_get(vertices, id_to_index(v1->edges[j].destination));
            v2->origins_count++;
        }
    }

    for (int i = 0; i < vertices->count; ++i) {
        v1 = vector_get(vertices, i);
        printf("Vertex %d is destination of %d \n", i + 1, v1->origins_count);
    }
}

/***
 * Populates list of origins into each vertex
 */
void pre_process_origins() {
    vertex *v1, *v2;
    // Tracks where is the end of each list
    int *index = malloc(sizeof(int) * vertices->count);
    int id;

    for (int i = 0; i < vertices->count; ++i) {
        v1 = vector_get(vertices, i);
        index[i] = 0;
        v1->origins = malloc(sizeof(vertex *) * v1->origins_count);
    }


    for (int i = 0; i < vertices->count; ++i) {
        v1 = vector_get(vertices, i);
        for (int j = 0; j < v1->edge_count; ++j) {
            id = v1->edges[j].destination - 1;
            v2 = vector_get(vertices, id);
            v2->origins[index[id]++] = v1;
        }
    }

    free(index);
}

/**
 * Calculates the cost of the path (one side to the other)
 *
 * @param path
 * @param reverse
 * @return
 */
float calculate_path_cost(vector *path, bool reverse) {
    vertex *v1, *v2;
    edge *e;
    float cost = 0;

    for (int i = 0; i < path->count - 1; ++i) {

        v1 = vector_get(path, i);
        v2 = vector_get(path, i + 1);

        if (reverse) {
            e = vertex_has_destination_vert(v2, v1);
        } else {
            e = vertex_has_destination_vert(v1, v2);
        }

        if (e != NULL) {
            cost += e->weight;
        } else {
            printf(RED "Could not find an edge to the next vertex, this should not happen in a path!\n" RESET);
        }
    }

    return cost;
}


/**
 * Prints compressible paths to screen
 */
void print_compressed_paths() {
    for (int i = 0; i < one_way_paths_found->count; ++i) {
        print_path(vector_get(one_way_paths_found, i));
        printf("Cost: %f\n\n", calculate_path_cost(vector_get(one_way_paths_found, i), false));
    }

    for (int i = 0; i < two_way_paths_found->count; ++i) {
        print_path(vector_get(two_way_paths_found, i));
        printf("Cost: %f\n", calculate_path_cost(vector_get(two_way_paths_found, i), false));
        printf("Cost: %f (reverse)\n\n", calculate_path_cost(vector_get(two_way_paths_found, i), true));
    }
}

/**
 * Serialize compressed path to file
 *
 * @param path
 * @param global_index
 * @param two_way
 */
void serialize_compressed_path(vector *path, int global_index, bool two_way) {
    printf(
            "p%d %d %f %f %d ",

            global_index,
            two_way ? 2 : 1,
            calculate_path_cost(path, false),
            two_way ? calculate_path_cost(path, true) : 0,
            path->count
    );
    fprintf(compresed_paths_file,
            "p%d %d %f %f %d ",

            global_index,
            two_way ? 2 : 1,
            calculate_path_cost(path, false),
            two_way ? calculate_path_cost(path, true) : 0,
            path->count
    );

    for (int i = 0; i < path->count; ++i) {
        printf("%d ", ((vertex *) vector_get(path, i))->id);
        fprintf(compresed_paths_file, "%d ", ((vertex *) vector_get(path, i))->id);
    }

    printf("\n");
    fprintf(compresed_paths_file, "\n");
}

/**
 * Iterates over each path found, and serializes it
 */
void serialize_compressed_paths() {

    printf("%d\n", one_way_paths_found->count + two_way_paths_found->count);
    fprintf(compresed_paths_file, "%d\n", one_way_paths_found->count + two_way_paths_found->count);

    int global_index = 1;

    for (int i = 0; i < one_way_paths_found->count; ++i) {
        serialize_compressed_path(vector_get(one_way_paths_found, i), global_index++, false);
    }

    for (int i = 0; i < two_way_paths_found->count; ++i) {
        serialize_compressed_path(vector_get(two_way_paths_found, i), global_index++, true);
    }
}

/**
 * Serializes compressed graph to file
 */
void serialize_graph() {
    vertex *v;

    int edge_count = 0;
    int max_degree = 0;

    // Computes max-degree and total vertex count
    for (int i = 0; i < vertices->count; ++i) {
        v = vector_get(vertices, i);

        edge_count += v->edge_count;

        // Check for new max vertex degree
        if (v->edge_count > max_degree) {
            max_degree = v->edge_count;
        }
    }

    // File header
    printf("%c %d %d %d\n", 'G', vertices->count, edge_count, max_degree);
    fprintf(compressed_output_file, "%c %d %d %d\n", 'G', vertices->count, edge_count, max_degree);

    // Serialize each vertex
    for (int i = 0; i < vertices->count; ++i) {
        v = vector_get(vertices, i);

        printf("%c %d %f %f %d ",
               'G',
               i + 1,
               v->latitude,
               v->longitude,
               v->edge_count
        );
        fprintf(compressed_output_file, "%c %d %f %f %d ",
                'G',
                i + 1,
                v->latitude,
                v->longitude,
                v->edge_count
        );

        // Serialize each edge from vertex
        for (int j = 0; j < v->edge_count; ++j) {
            printf("%d %f ",
                   v->edges[j].destination,
                   v->edges[j].weight
            );
            fprintf(compressed_output_file, "%d %f ",
                    v->edges[j].destination,
                    v->edges[j].weight
            );
        }

        printf("\n");
        fprintf(compressed_output_file, "\n");
    }
}

/********************
 * GRAPH REBUILDING *
 ********************/

/**
 * Clears memory from edge information from a list of vertices
 *
 * @param list
 */
void wipe_internal_vertices(vector *list) {
    vertex *v;

    for (int i = 1; i < list->count - 1; ++i) {
        v = vector_get(list, i);

        printf(MAGENTA "Wiping vertex: %d\n" RESET, v->id);

        wiped_vertices++;
        wiped_edges += v->edge_count;

        v->edge_count = 0;
        free(v->edges);
    }
}

/**
 * Rebuilds path using a new artificial vertex
 *
 * @param path
 * @param weight
 * @param two_way
 */
void rebuild_path(vector *path, bool two_way) {
    vertex *head, *tail, *sub;
    float forward_cost = 0;
    float reverse_cost = 0;
    head = vector_get(path, 0);
    tail = vector_get(path, path->count - 1);

    printf("Allocating new replacement vertex\n");

    forward_cost = calculate_path_cost(path, false);
    if (two_way) {
        reverse_cost = calculate_path_cost(path, true);
    }

    sub = malloc(sizeof(vertex));
    sub->id = vertices->count + 1;
    sub->longitude = 0;
    sub->latitude = 0;
    if (two_way) {
        sub->edge_count = 2;
        sub->edges = malloc(sizeof(edge) * sub->edge_count);
        sub->edges[0].destination = tail->id;
        sub->edges[0].weight = forward_cost / 2;
        sub->edges[1].destination = head->id;
        sub->edges[1].weight = reverse_cost / 2;
    } else {
        sub->edge_count = 1;
        sub->edges = malloc(sizeof(edge) * sub->edge_count);
        sub->edges[0].destination = tail->id;
        sub->edges[0].weight = forward_cost;
    }

    printf("Adding to global vertex path\n");

    vector_add(vertices, sub);

    printf("Fixing head edges\n");
    for (int i = 0; i < head->edge_count; ++i) {
        if (in_path(head->edges[i].destination, path)) {
            head->edges[i].destination = sub->id;
            head->edges[i].weight = forward_cost / 2;
            break;
        }
    }

    printf("Fixing tail edges\n");
    for (int i = 0; i < tail->edge_count; ++i) {
        if (in_path(tail->edges[i].destination, path)) {
            tail->edges[i].destination = sub->id;
            tail->edges[i].weight = reverse_cost / 2;
            break;
        }
    }
}

/**
 * Compresses path inside graph
 *
 * @param path
 * @param weight
 * @param two_way
 */
void compress_path(vector *path, float weight, bool two_way) {
    print_path(path);

    wipe_internal_vertices(path);

    rebuild_path(path, two_way);
}

/**
 * Iterates each found path and compress it
 */
void compress_paths() {
    vector *path;
    float weight;

    printf(GREEN);
    spam("RUNNING TWO-WAY-PATH COMPRESSING PASS");
    printf(RESET);

    for (int i = 0; i < two_way_paths_found->count; ++i) {
        printf(GREEN "Compressing two-way-path [ %d / %d ]\n" RESET, i + 1, two_way_paths_found->count);

        path = vector_get(two_way_paths_found, i);

        weight = calculate_internal_edge_weights(path);

        compress_path(path, weight / 4, true);
    }

    spam(GREEN "RUNNING ONE-WAY-PATH COMPRESSING PASS" RESET);

    for (int i = 0; i < one_way_paths_found->count; ++i) {
        printf(GREEN "Compressing one-way-path [ %d / %d ]\n" RESET, i + 1, one_way_paths_found->count);

        path = vector_get(one_way_paths_found, i);

        weight = calculate_internal_edge_weights(path);

        compress_path(path, weight / 2, false);
    }
}

/***********
 * M A I N *
 ***********/

/**
 * Initializes files and variables
 *
 * @param arg_count
 * @param args
 */
void initialize(int arg_count, char **args) {
    for (int i = 0; i < arg_count; ++i) {
        printf("%d: %s\n", i, args[i]);
    }

    // Assert 4 arguments in initialization
    if (arg_count != 4) {
        printf("Expecting 4 arguments, usage: <input_file_path> <compressed_graph_output_path> <compressed_paths_output>\n");
        exit(1);
    }

    // Save paths used
    input_path = args[1];
    compressed_graph_path = args[2];
    compressed_paths_path = args[3];

    // Open files
    compressed_output_file = fopen(compressed_graph_path, "w");
    compresed_paths_file = fopen(compressed_paths_path, "w");
    input_file = fopen(input_path, "r");
}

int main(int arg_count, char **args) {

    spam("Processing arguments");
    initialize(arg_count, args);

    spam("Parsing Graph File to memory!");
    parse_graph_file();

    spam("Pre-processing origins count!");
    pre_process_origins_count();

    spam("Pre-processing origins!");
    pre_process_origins();

    spam("Searching compressible paths!");
    search_compressible_paths();

    spam("Printing compressed paths");
    print_compressed_paths();

    spam("Serializing compressed paths");
    serialize_compressed_paths();

    spam("Compressing paths");
    compress_paths();

    spam("Serializing compressed graph");
    serialize_graph();

    spam("Printing graph");
    print_graph();


    printf(GREEN);
    spam("FINISHED SUCCESSFULLY!");
    printf(RESET);

    return 0;
}
