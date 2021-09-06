# panorama
![image](https://github.com/fallantbell/panorama/blob/main/image/panorama.jpg)  
This is the panorama project in c++ with opencv  
In this project, it only accept the sequential picture from left to right, or it will be wrong.  

There are some steps to do panorama below  
## 1. Do the Cylindrical Projection  
This is very important step to stitch image into panorama  
If you dont do this, it will cause distortion when you stitch the image in one direction  

I find the paper about this :  
https://pdfs.semanticscholar.org/936b/0f1bf4dd6323ccf9d362efc15fef6c8119af.pdf

I follow this code to do cylindrical projection :  
https://stackoverflow.com/questions/12017790/warp-image-to-appear-in-cylindrical-projection?lq=1  

### panorama with cylindrical projection  
<img src="https://github.com/fallantbell/panorama/blob/main/image/with_cylindrical.png" width="400" height="300">  

### panorama without cylindrical projection
<img src="https://github.com/fallantbell/panorama/blob/main/image/without_cylindrical.png" width="400" height="300">  

### cylindrical projection  
<img src="https://github.com/fallantbell/panorama/blob/main/image/cylindrical_projection.png" width="500" height="300">  

## 2. Find the keypoint and descriptor  
There are many ways to do this, such as: SIFT, SURF, ORB ... [document](https://docs.opencv.org/4.5.2/db/d27/tutorial_py_table_of_contents_feature2d.html)  
In my case, I use `fast algorithm` to find keypoint.  
Because I think it is enough for me, and it is faster than other algorithm  

### keypoint  
<img src="https://github.com/fallantbell/panorama/blob/main/image/keypoint.png" width="400" height="300">  

## 3. Find the match between two image
After finding keypoint and decriptor of two images  
We need to find match between them  
There are also many algorithm , if you are interested, you can check the [document](https://docs.opencv.org/2.4/modules/features2d/doc/common_interfaces_of_descriptor_matchers.html)  
And I use `FlannBasedMatcher` here  

### match  
<img src="https://github.com/fallantbell/panorama/blob/main/image/match.png" width="400" height="300">  

## 4. Find the good match  
After finding match  
We will get match between two images  
But actually we dont need so many match  
And there are some match may be wrong  
Thus, we only reserver some better match into good match  

### good match  
<img src="https://github.com/fallantbell/panorama/blob/main/image/goodmatch.png" width="400" height="300">  

## 5. Find the translation matrix  
In this step, we need to find the translation matrix between the match points  
Instead of using findhomography API, I write a little function with `RANSAC algorithm` to find that 
## 6. Fitting the tranlation matrix  
I dont use `warpPerspective API` prevent from the distortion  
Instead, I use `warpAffine API`  

### After translation  
<img src="https://github.com/fallantbell/panorama/blob/main/image/translate.png" width="400" height="400">  

## 7. Stitch two image with blending  
In this step, I check every pixels of two images  

*both of two images are black*  
=> set this pixel to black  

*one of two images is black*  
=> set this pixel to another  

*both of two images are not black*  
=> if this pixel is closer to one image , set the higher weight to the image  

### with blending  
<img src="https://github.com/fallantbell/panorama/blob/main/image/blending.png" width="400" height="400">  

### without blending  
<img src="https://github.com/fallantbell/panorama/blob/main/image/without_blending.png" width="400" height="400">  

## Crop the image  
After stitching the images, it remain some black region  
So we need to remove them  

### without crop
<img src="https://github.com/fallantbell/panorama/blob/main/image/without_crop.png" width="800" height="400">  

### crop
<img src="https://github.com/fallantbell/panorama/blob/main/image/crop.png" width="800" height="400">  

## Result  
### ocean  
stitch 10 images (720\*960)  
time: 869 ms  


### pond  
stitch 9 images (720\*960)  
time: 1363 ms  
![image](https://github.com/fallantbell/panorama/blob/main/image/panorama.jpg)  
