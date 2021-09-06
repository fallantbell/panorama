# panorama
![image](https://github.com/fallantbell/panorama/blob/main/image/panorama.png)  
This is the panorama project in c++ with opencv  
In this project, it only accept the sequential picture from left to right, or it will be wrong.  

There is some explanation below  
## 1. Do the Cylindrical Projection  
This is very important step to stitch image into panorama  
If you dont do this, it will cause distortion when you stitch the image in one direction  

I find the paper about this :  
https://pdfs.semanticscholar.org/936b/0f1bf4dd6323ccf9d362efc15fef6c8119af.pdf

I follow this code to do cylindrical projection :  
https://stackoverflow.com/questions/12017790/warp-image-to-appear-in-cylindrical-projection?lq=1  

### image with cylindrical projection  
<img src="https://github.com/fallantbell/panorama/blob/main/image/with_cylindrical.png" width="400" height="300">  

### image without cylindrical projection
<img src="https://github.com/fallantbell/panorama/blob/main/image/without_cylindrical.png" width="400" height="300">  


## 2. Find the keypoint and descriptor  
There are many ways to do this, such as: SIFT, SURF, ORB ... [document](https://docs.opencv.org/4.5.2/db/d27/tutorial_py_table_of_contents_feature2d.html)  
In my case, I use fast algorithm to find keypoint.  
Because I think it is enough for me, and it is faster than other algorithm  
## 3. Find the match between two image
After finding keypoint and decriptor of two images  
You need to find match between them  
There are also many algorithm , if you are interested, you can check the [document](https://docs.opencv.org/2.4/modules/features2d/doc/common_interfaces_of_descriptor_matchers.html)  
And I use FlannBasedMatcher here  
## 4. Find the good match  
After finding match  
We will get match points between two images  
But actually we dont need so many match points  
Thus, we only reserver some better match into good match  
## 5. Find the translation matrix  
In this step, we need to find the translation matrix between the match points  
Instead of using findhomography API, I write a little function with RANSAC algorithm to find that 
## 6. Fitting the tranlation matrix  
I dont use warpPerspective API here prevent from the distortion  
Instead, I use warpAffine API  
## 7. Stitch two image  

