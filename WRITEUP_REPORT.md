# MP.0 Mid-term report



### MP.1 Data Buffer Optimization
I did not implement the ring buffer using vectors because I read that vector is not efficient for erasing and inserting operations. These operation are needed for ring buffer, after google searching I found that there is a ring buffer implementation in boost library. The name of the class is  circular_buffer and it supports most of methods of vector class. So I decided to use this class, please see line 20 and 45

### MP.2 Keypoint Detection
I used if else statement to select between the options


### MP.3 Keypoint Removal
I used "contains" method of rect class to verify is the keypoint inside the vechicle rectangle


### MP.4 Keypoint Descriptors
I used a switch statement and a helper function to select between the possible descriptors

### MP.5 Descriptor Matching
I used if else statements to select the matching strategy.
I had to add an extra "if" statement to differentiate between HOG and binary descriptors as NORM_HAMMING can be used only on binary descriptors


### MP.6 Descriptor Distance Ratio
I iterated through the matches to test the distance ratio

### MP.7 Performance Evaluation 1
Please see file Performance_reports/MP7_nbr_kpts_in_vehic.pdf 

### MP.8 Performance Evaluation 2
Please see file Performance_reports/MP8_nbr_matched_points.pdf

### MP.8 Performance Evaluation 2
Please see file Performance_reports/MP8_nbr_matched_points.pdf

### MP.9 Performance Evaluation 3
Please see files:

 - Performance_reports/MP9_1_keypts_detect_time.pdf
 
 - Performance_reports/MP9_2_kpts_descrip_ext_time.pdf
 
 - Performance_reports/MP9_3_Conclusions.pdf
 
For more details you can also see the file Performance_reports/detect_desript_performance_evaluation.xlsx




