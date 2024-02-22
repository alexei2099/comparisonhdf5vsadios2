include <stdio.h>
include <stdlib.h>
include <time.h>
include <adios2_c.h>
include "hdf5.h"

define NX 1000
define NY 1000
define FILENAME "data_comparison"

void generate_data(int data[NX][NY]) {
    srand(time(NULL));
    for (int i = 0; i < NX; ++i) {
        for (int j = 0; j < NY; ++j) {
            data[i][j] = rand() % 100; // Generate random data
        }
    }
}

void write_hdf5(int data[NX][NY]) {
    hid_t file_id, dataset_id, dataspace_id;
    hsize_t dims[2] = {NX, NY};

    // Create a new file using default properties.
    file_id = H5Fcreate(FILENAME "_hdf5.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Create the data space for the dataset.
    dataspace_id = H5Screate_simple(2, dims, NULL);

    // Create a dataset in the file.
    dataset_id = H5Dcreate2(file_id, "dataset", H5T_STD_I32LE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    // Write the data to the dataset.
    H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    // Close resources
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);
}

void write_adios2(int data[NX][NY]) {
    adios2_adios* ad = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);

    adios2_io* io = adios2_declare_io(ad, "IO");
    adios2_define_variable(io, "data", adios2_type_int32_t, 2, NULL, NULL, NULL, adios2_constant_dims_true);

    adios2_engine* engine = adios2_open(io, FILENAME "_adios2.bp", adios2_mode_write);

    adios2_begin_step(engine, adios2_step_mode_append, -1, 1);
    adios2_put(engine, "data", data, adios2_mode_deferred);
    adios2_end_step(engine);

    adios2_close(engine);
    adios2_finalize(ad);
}

void read_hdf5() {
    hid_t file_id, dataset_id, dataspace_id;
    int data_out[NX][NY];

    // Open the file and the dataset.
    file_id = H5Fopen(FILENAME "_hdf5.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
    dataset_id = H5Dopen2(file_id, "dataset", H5P_DEFAULT);

    // Read the data.
    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_out);

    // Close resources.
    H5Dclose(dataset_id);
    H5Fclose(file_id);
}

void read_adios2() {
    adios2_adios* ad = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);

    adios2_io* io = adios2_declare_io(ad, "IO");
    adios2_engine* engine = adios2_open(io, FILENAME "_adios2.bp", adios2_mode_read);

    adios2_begin_step(engine, adios2_step_mode_read, 0, -1);
    const adios2_variable* var = adios2_inquire_variable(io, "data");
    int data_out[NX][NY];
    adios2_get(engine, var, data_out, adios2_mode_deferred);
    adios2_end_step(engine);

    adios2_close(engine);
    adios2_finalize(ad);
}

int main() {
    int data[NX][NY];
    generate_data(data);

    printf("Dataset size: %d x %d\n", NX, NY);

    printf("Writing HDF5...\n");
    clock_t start = clock();
    write_hdf5(data);
    clock_t end = clock();
    printf("HDF5 write time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    printf("Writing ADIOS 2...\n");
    start = clock();
    write_adios2(data);
    end = clock();
    printf("ADIOS 2 write time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    printf("Reading HDF5...\n");
    start = clock();
    read_hdf5();
    end = clock();
    printf("HDF5 read time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    printf("Reading ADIOS 2...\n");
    start = clock();
    read_adios2();
    end = clock();
    printf("ADIOS 2 read time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

    return 0;
}
