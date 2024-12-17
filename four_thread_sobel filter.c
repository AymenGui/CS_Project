#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

// Define the number of threads
#define NUM_THREADS 4

// Define the image structure
unsigned char* input_image;
unsigned char* output_image;
int width, height;

// Mutex to protect pixel access
pthread_mutex_t mutex;

// Sobel filter kernels
int Gx[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int Gy[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

// Function to apply Sobel filter to a specific pixel
void* sobel_filter_pixel(void* arg) {
    int thread_id = *((int*)arg);  // Thread ID

    // Calculate the range of pixels each thread processes
    int rows_per_thread = height / NUM_THREADS;
    int start_row = thread_id * rows_per_thread;
    int end_row = (thread_id == NUM_THREADS - 1) ? height : start_row + rows_per_thread;

    for (int y = start_row + 1; y < end_row - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0;
            int sumY = 0;

            // Apply Sobel kernel to the neighborhood of the pixel
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int pixel = input_image[(y + i) * width + (x + j)];
                    sumX += pixel * Gx[i + 1][j + 1];
                    sumY += pixel * Gy[i + 1][j + 1];
                }
            }

            int magnitude = (int)sqrt(sumX * sumX + sumY * sumY);
            if (magnitude > 255) {
                magnitude = 255;
            }

            // Protect the pixel write operation with a mutex
            pthread_mutex_lock(&mutex);
            output_image[y * width + x] = (unsigned char)magnitude;
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(NULL);
}

// Function to write the output image to a PGM file
void write_image(const char* fileName, const unsigned char* image, int width, int height) {
    FILE* file = fopen(fileName, "wb");
    if (!file) {
        printf("Error opening file for writing\n");
        return;
    }

    // Write P5 PGM header
    fprintf(file, "P5\n%d %d\n255\n", width, height);

    // Write pixel data
    fwrite(image, 1, width * height, file);

    fclose(file);
}

// Function to read the input image from a PGM file
unsigned char* read_image(const char* fileName, int* width, int* height) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Error opening the file!\n");
        return NULL;
    }

    char magic_number[3];
    if (fgets(magic_number, sizeof(magic_number), file) == NULL) {
        fprintf(stderr, "Error reading magic number\n");
        fclose(file);
        return NULL;
    }

    skip_comments(file);

    if (fscanf(file, "%d %d", width, height) != 2) {
        fprintf(stderr, "Error reading width and height\n");
        fclose(file);
        return NULL;
    }

    skip_comments(file);

    int max_gray;
    if (fscanf(file, "%d", &max_gray) != 1) {
        fprintf(stderr, "Error reading max gray value\n");
        fclose(file);
        return NULL;
    }

    fgetc(file);  // Skip the newline after max gray value

    int image_dim = (*height) * (*width);
    unsigned char* image = (unsigned char*)malloc(image_dim * sizeof(char));
    if (!image) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    if (fread(image, 1, image_dim, file) != image_dim) {
        printf("Error reading pixel data\n");
        free(image);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return image;
}

// Function to skip comments in the PGM file
void skip_comments(FILE* file) {
    char ch;
    while ((ch = fgetc(file)) == '#') {
        while ((ch = fgetc(file)) != '\n' && ch != EOF);  // Skip to the end of the line
    }
    ungetc(ch, file);  // Put the non-comment character back into the stream
}

int main() {
    int width, height;

    // Read input image
    input_image = read_image("input.pgm", &width, &height);
    if (input_image == NULL) {
        return -1;
    }
    printf("Input Image Dimensions: %dx%d\n", width, height);

    // Allocate memory for the output image
    output_image = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    if (!output_image) {
        fprintf(stderr, "Memory allocation failed\n");
        free(input_image);
        return -1;
    }

    // Initialize the mutex
    pthread_mutex_init(&mutex, NULL);

    // Create threads
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, sobel_filter_pixel, (void*)&thread_ids[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Write the output image
    write_image("output.pgm", output_image, width, height);
    printf("Sobel filter applied and saved as output.pgm\n");

    // Free memory and destroy the mutex
    free(input_image);
    free(output_image);
    pthread_mutex_destroy(&mutex);

    return 0;
}
