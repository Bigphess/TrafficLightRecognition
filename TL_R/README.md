# Traffic Light Recognition
* This is a Graduation project in 2018 in BIT, about traffic light recognition based on image features and simple KNN.
* This algorithm can detect the position and color of the traffic light.

**The code is in ``/TL_R/TL_R/TLR.cpp`` and ``/TL_R/TL_R/main.cpp``**, the test video in ``main.cpp`` is belong to BIT, not in this repository.


## General Steps:
* Use the color, shape features, etc, to isolate the ROI of traffic light.
* Put these ROIs into KNN and classify.

## Specific steps:
### Original image in an intersection
![ori image](res/1.jpg)

As you can see, there are four different lights in the image.

### Color feature
* Use HSV color space to seperate the light part of the traffic light
* Use RGB color space to seperate the box of traffic light(because the black color is hard to seperate from HSV color space)
* Choose the top half as the ROI because traffic lights always on the top half

![color](res/2.jpg)

![f1](res/f1.jpg)

### Morphological operations
* Isolate the traffic light box and reduce noise
* Operations:
	* Open
	* Close 
	* Shape of the convolution kernel: Ellipse
	* Size of the kernel: 3x3

![mor](res/3.jpg)

### Shape features
* Use shape (rectangle) and concaveness to get the candidate traffic light areas


Formulas about shape:
![f2](res/f2.jpg)

Formulas about concaveness:
![f3](res/f3.jpg)

Combine them:
![f4](res/f4.jpg)

Result after this step:
![cand](res/4.jpg)


### Judge the result
* Use HSV and shape to decided whether it is a traffic light or not
* Convert to binary image

![f5](res/f5.jpg)

I put the binary images onto the origin image:
![box](res/5.jpg)


### KNN
* Use KNN to classify the lights
	* Because we have already had the binary image of each of the traffic light box, it is easy to do the classification on KNN
* Class: 2(red & green)
* Size of the image: 25x40
* Training set, 200 images, 100 green and 100 red
* Test set: 120 images, 60 green and 60 red
* Error rate: 3.33%


## Final Result
As you can see, the final result is as follow:
![result](res/6.jpg)

* When the road situation is good, the accuracy will be 90%. The good situation is included:
	* Without many trees
	* The background is sky
	* The traffic light is big enough
	* In daytime
* The speed is 22ms/image


## Discussion
### Advantages
* This algorithm can have a good result in good situation without deep learning(labeling for traffic light is very hard and lights are also too small in the whole image sometimes)
* In the KNN demo, I only use two classes for training, red and green. If we add more classes into the training, it will recognize more classes like arrow.

### Limitation
* This algorithm can only work in good situation, because it is based on the box of traffic light, if it is night or there are many trees, the box and the background will be merged.
* The accuracy is not enought for using in practice.
* The real road test do not have a good performerce.
* I used the experienced number of color threshold, instead of histogram,this will effect the final result.
* Because it is my first time programming, the style of coding is not good.
