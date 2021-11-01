/*
Author: Travis Schrader
Date: 12/10/20
This program takes in a ppm image and then applies a laplacian filter on it using 4 threads. It then print out the elapsed time taken. 
*/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>


#define THREADS 1

#define filterWidth 3
#define filterHeight 3

#define RGB_MAX 255


typedef struct {
         unsigned char r, g, b;
} PPMPixel;

struct parameter {
        PPMPixel *image;         //original image
        PPMPixel *result;        //filtered image
        unsigned long int w;     //width of image
        unsigned long int h;     //height of image
        unsigned long int start; //starting point of work
        unsigned long int size;  //equal share of work (almost equal if odd)
};


/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    (1) For each pixel in the input image, the filter is conceptually placed on top ofthe image with its origin lying on that pixel.
    (2) The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    (3) The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.

 */
void *threadfn(void *params)
{

	struct parameter thr_params;
	thr_params = *(struct parameter*)params;


	int laplacian[filterWidth][filterHeight] =
        {
          -1, -1, -1,
          -1,  8, -1,
          -1, -1, -1,
        };

	/*For all pixels in the work region of image (from start to start+size)
          Multiply every value of the filter with corresponding image pixel. Note: this is NOT matrix multiplication.
          Store the new values of r,g,b in p->result.
       */


	for(int x = 0; x < thr_params.w; x++){
		for(int y = thr_params.start; y < thr_params.start + thr_params.size; y++){
			int red, green, blue;
			red = 0;
			green = 0;
			blue = 0;
			for(int filterx = 0; filterx < filterWidth; filterx++){
				for(int filtery = 0; filtery < filterHeight; filtery++){
					int imagex = (x - (filterWidth/2) + filterx + thr_params.w) % thr_params.w;
					int imagey = (y - (filterHeight/2) + filtery + thr_params.h) % thr_params.h;

					red += thr_params.image[imagey * thr_params.w + imagex].r * laplacian[filterx][filtery];
					green += thr_params.image[imagey * thr_params.w + imagex].g * laplacian[filterx][filtery];
					blue += thr_params.image[imagey * thr_params.w + imagex].b * laplacian[filterx][filtery];

				}
			}
			if(red < 0) {red = 0;}
                        if(red > 255) {red = 255;}

                        if(green < 0) {green = 0;}
                        if(green > 255){green = 255;}

                        if(blue < 0){ blue = 0;}
                        if(blue > 255){blue = 255;}

			thr_params.result[y * thr_params.w + x].r = red;
			thr_params.result[y * thr_params.w + x].g = green;
			thr_params.result[y * thr_params.w + x].b = blue;
		}
	}



        return NULL;
}

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "name" (the second argument).
 */
void writeImage(PPMPixel *image, char *name, unsigned long int width, unsigned long int height)
{
	// Setting up variables
	FILE *fp;
	char outputfile[128];

	// Setting up my new file name
	strcpy(outputfile, name);
        strtok(outputfile, ".");
        strcat(outputfile, "_laplacian.ppm");

	// Creating new file
	fp = fopen(outputfile, "wb");

	// Checking for file open error
	if(fp == NULL){
		printf("Error opening file\n");
		exit(1);
	}

	// Writing header
	fprintf(fp, "P6\n");
	fprintf(fp, "%ld", width);
	fprintf(fp," %ld\n", height);
	fprintf(fp, "255\n");

	// Writing binary
	fwrite(image,1,height * width * 3, fp);

	if(fwrite == 0){
		printf("Error writing to file.");
		exit(1);
	}
	fclose(fp);
}



/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height
    255                 -- max color value

 Check if the image format is P6. If not, print invalid format error message.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename)
 */
PPMPixel *readImage(const char *filename, unsigned long int *width, unsigned long int *height)
{
        FILE* Rfile;
        char format[10];
	char Iwidth[5];
        char Iheight[5];
        char color[5];
        int lastSeen = 2;
        char curr;
        struct parameter *img;
        img = malloc(sizeof(struct parameter));


        //opening file passed
        Rfile = fopen(filename, "rb");
        if(Rfile == NULL){
                fprintf(stderr, "Error Opening file.");
		exit(1);
        }

	// Handling format
        fscanf(Rfile, "%s", format);

	if(!strcmp(&format[0], "P6\n")){
		printf("Invalid file type");
		exit(1);
	}
	// Moving file past end of file
        fgetc(Rfile);
        fgetc(Rfile);

	// moving past the comments
        while(curr != '\n'){
                curr = fgetc(Rfile);
        }

	//Getting width, height, and color
        fscanf(Rfile, "%ld", &img->w);
        fscanf(Rfile, "%ld", &img->h);
        fscanf(Rfile, "%s", color);

	*width = img->w;
	*height = img->h;

	if(atoi(color) != 255){
		fprintf(stderr, "Error invalid color mode.");
		exit(1);
	}


	fgetc(Rfile); //moving off new line char

	//allocating memory for my pixels
        img->image = malloc(img->w * img->h * 3);

	// Reading image
        if(!fread(img -> image, 1, img->h*img->w*3, Rfile)){
        	fprintf(stderr, "Error reading image.");
		exit(1);
	}

        fclose(Rfile);

        return img -> image;
}



/* Create threads and apply filter to image.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {

	// Declaring necessary variables
	PPMPixel *result;
	int work = h / THREADS;
	result = malloc( w * h * 3);
	struct parameter params[THREADS];
	pthread_t threads[THREADS];
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for(int i = 0; i < THREADS; i++){
		// allocating memory for temp structs to be passed to the thread function
		params[i] = *(struct parameter*)malloc(sizeof(struct parameter));
		params[i].image = image;
		params[i].result = result;
		params[i].w = w;
		params[i].h = h;
		params[i].start = i * work;
		if(i == THREADS -1){
			params[i].size = h - (work * i);
		}
		else{
			params[i].size = work;
		}
		// execute new threads
		int ret = pthread_create(&threads[i], NULL, threadfn, &params[i]);
		if(ret != 0){
			printf("Error creating thread");
			exit(1);
		}
	}

	// wait for threads to finish
	for(int j = 0; j < THREADS; j++){
		void* ret;
		pthread_join(threads[j], &ret);
	}

	gettimeofday(&end, NULL);
	*elapsedTime = (double)((end.tv_sec / 1000000 + end.tv_usec) - (start.tv_sec / 1000000 + start.tv_usec)) / 1000000;

        return result;
}




int main(int argc, char *argv[])
{
        unsigned long int w, h;
        double elapsedTime = 0.0;
	struct parameter *img;

	if(argc < 2 || argc > 3){
		printf("Usage ./a.out filename\n");
		exit(1);
	}

	img -> image = readImage(argv[1], &w, &h);


	img -> result = apply_filters(img->image, w, h, &elapsedTime);


	writeImage(img -> result, argv[1], w, h);

	// Freeing allocated memory
	free(img->result);
	free(img->image);

	// Showing elapsed time
	printf("Elapsed Time: %0.3lf \n", elapsedTime);



        return 0;
}
