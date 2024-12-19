#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
// Définir la structure pour les arguments des threads
typedef struct {
    const unsigned char* input_image;
    unsigned char* output_image;
    int width;
    int height;
    int start_row;
    int end_row;
} ThreadData;

// Sobel filter pour une portion de l'image
void* sobel_filter_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    const unsigned char* input_image = data->input_image;
    unsigned char* output_image = data->output_image;
    int width = data->width;
    int start_row = data->start_row;
    int end_row = data->end_row;

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

    for (int y = start_row+1; y < end_row-1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0;
            int sumY = 0;

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

            output_image[y * width + x] = (unsigned char)magnitude;
        }
    }

    return NULL;
}

void skip_comments(FILE* file) {
	char ch;
	while ((ch = fgetc(file)) == '#') { // If the line starts with '#', it's a comment
		printf("comment present");
		while ((ch = fgetc(file)) != '\n' && ch != EOF); // Skip to the end of the line
	}
	ungetc(ch, file); // Put the non-comment character back into the stream
}
//function to write image to a pgm file
void write_image(const char* fileName, const unsigned char* image, int width, int height) {
	FILE* file = fopen(fileName, "wb");
	if (!file) {
		printf( "Error opening file for writing\n");
		return;
	}

	// Write P5 PGM header
	fprintf(file, "P5\n%d %d\n255\n", width, height);

	// Write pixel data
	fwrite(image, 1, width * height, file);

	fclose(file);
}

//function to read image to a pgm file
unsigned char* read_Image(const char* fileName, int* width, int* height) {
	
	FILE* file = fopen(fileName, "rb");
	if (file == NULL) {
		printf("Error Opening the file!");
	}
	
	char magic_number[3];
	if (fgets(magic_number, sizeof(magic_number), file)==NULL) {
		fprintf(stderr, "Error reading magic number\n");
		fclose(file);
		return 1;
	}

	
	printf("%s\n", magic_number);

	skip_comments(file);

	
	if (fscanf(file, "%d %d", width, height) != 2) {
		fprintf(stderr, "Error reading width and height\n");
		fclose(file);
		return 1;
	}

	// Skip comments and read max gray value
	skip_comments(file);

	int max_gray;
	if (fscanf(file, "%d", &max_gray) != 1) {
		fprintf(stderr, "Error reading max gray value\n");
		fclose(file);
		return 1;
	}
 
	if (max_gray > 255) {
		fprintf(stderr, "Unsupported max gray value > 255\n");
		fclose(file);
		return 1;
	}

	// skip the single whitespace after the max_gray after it gave me hard time
	fgetc(file);

	// allocate contiguous block memory for 1D aarray
	int image_dim = (*height) * (*width);
	unsigned char* image = (unsigned char*)malloc(image_dim * sizeof(char));
	if (!image) {
		fprintf(stderr, "Memory allocation failed\n");
		fclose(file);
		return NULL;
	}
	
	//read the entire bloc of binary data (image_dim) of 1byte size from the stream file
	if (fread(image, 1, image_dim, file) != image_dim) {
		printf( "Error reading pixel data\n");
		free(image);
		fclose(file);
		return 1;
	}
	
	
	fclose(file);
	return image;

}


int main() {
    int width, height, numb_thread = 6;

    unsigned char* input_image = read_Image("img0024.pgm", &width, &height);
    if (input_image) {
		printf("Input_Image Dimensions: %dx%d\n", width, height);
    }

    unsigned char* output_image = (unsigned char*)malloc(width * height);
    if (!output_image) {
        printf("Memory allocation failed\n");
        free(input_image);
        return -1;
    }

    // Initialize threads
    pthread_t threads[numb_thread];
    ThreadData thread_data[numb_thread];

    // Calculate rows per thread, each thread will be assigned to a portion of the 2D image Array
	//the image portion to be assgned is computed using Rows/numb_thread 
    int rows_per_thread = height / numb_thread;
    int remaining_rows = height % numb_thread; // Remaining rows if not evenly divisible

    int current_start_row = 0;
	
	//clock_t start_time = clock();
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time); // Start timing
	
	//Putting threads to work
    for (int i = 0; i < numb_thread; i++) {
        thread_data[i].input_image = input_image;
        thread_data[i].output_image = output_image;
        thread_data[i].width = width;
        thread_data[i].height = height;

        thread_data[i].start_row = current_start_row;

        // Assign the rows for the thread
        thread_data[i].end_row = current_start_row + rows_per_thread;

        if (i == numb_thread - 1) {
            // The last thread takes the remaining rows
            thread_data[i].end_row += remaining_rows;
        }

        //printf("start row thread %d: %d \n",i,thread_data[i].start_row);
        //printf("end row thread %d: %d \n",i,thread_data[i].end_row);
        
        current_start_row = thread_data[i].end_row-2;

        // Create the thread
        if (pthread_create(&threads[i], NULL, sobel_filter_thread, &thread_data[i]) != 0) {
            printf( "Error creating thread %d\n", i);
            free(input_image);
            free(output_image);
            return -1;
        }
    }

    // Main wait for all threads to complete
    for (int i = 0; i < numb_thread; i++) {
        pthread_join(threads[i], NULL);
    }
	
	// clock_t end_time = clock();
     clock_gettime(CLOCK_MONOTONIC, &end_time); // End timing
	 
    // Calculate execution time in seconds
    double execution_time = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
							
							
    printf("Execution Time: %.4f seconds\n", execution_time);
	

    // Write the output image
    write_image("output_multithread.pgm", output_image, width, height);

    // Free memory
    free(input_image);
    free(output_image);

    return 0;
}