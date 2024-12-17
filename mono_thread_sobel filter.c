#include<stdio.h>
#include<stdlib.h>
#include<math.h>


/* 

A PGM (Portable Gray Map) P5 image is a grayscale image format used in the Netpbm format family. 
The P5 format is a binary encoding of the PGM image, where the pixels are represented in binary,
the pgm file contain a header in ASCII and binary data (8 bit) values between 0 and 255
Example of PGM P5 Header:

P5
# Optional comment line
256 256
255

this program apply sobel filter for edge detection on pgm image file and does not preview image
to preview output image please use a web viewer: https://smallpond.ca/jim/photomicrography/pgmViewer/index.html
 
the sobel filter computes derivative of x and y direction and then computes the magnitude
each pixel needs 9 neighbor so  the sobel filter fuction does not process the image limits

*/





//PGM image file may contain comments so wrote this function to skip comments and skip head aches!!
void skip_comments(FILE* file) {
	char ch;
	while ((ch = fgetc(file)) == '#') { // If the line starts with '#', it's a comment
		printf("comment present");
		while ((ch = fgetc(file)) != '\n' && ch != EOF); // Skip to the end of the line
	}
	ungetc(ch, file); // Put the non-comment character back into the stream
}

//sobel filter to be applied to the input_image ,just a convolurion operation in time , very simple
void sobel_filter(const unsigned char* input_image, unsigned char* output_image, int width, int height) {
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

	for (int y = 1; y < height - 1; y++) {
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
	int width, height;
    //width number of col
	//hight number of rows
	
	//to read the image provide the path (this tested path exemple is in windows)
	unsigned char* input_image = read_Image("D:/Cpp/Image_Pro/MultiThread_Image/x64/Debug/sample.pgm", &width, &height);

	if (input_image) {
		printf("Input_Image Dimensions: %dx%d\n", width, height);
				
	}

	// Allocate memory for the output image
	unsigned char* output_image = (unsigned char*)malloc(width * height);
	if (!output_image) {
		fprintf(stderr, "Memory allocation failed\n");
		free(input_image);
		return -1;
	}

	// Apply Sobel filter
	sobel_filter(input_image, output_image, width, height);

	// Write the output image
	write_image("output.pgm", output_image, width, height);

	

	// Free memory
	free(input_image);
	free(output_image);


	getchar();
	return 0;
	
}