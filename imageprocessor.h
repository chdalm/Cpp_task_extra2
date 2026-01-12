#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <map>
using namespace std;

struct Cut {
    cv::Point2f p_out;
    cv::Point2f p_in;
    int contour_out;
    int contour_in;
    
    float length() const {
        return cv::norm(p_out - p_in);
    }
};

class ImageProcessor {
public:
    ImageProcessor() = default;
    
    void process(const cv::Mat &binImage);
    
    vector<vector<cv::Point2f>> mergedContours();
    
    float totalCutsLength() const;
    
    string getInfoString() const;
    
    const  vector<vector<cv::Point>>& getContours() const { return contours; }
    const  vector<cv::Vec4i>& getHierarchy() const { return hierarchy; }
    const  vector<Cut>& getCuts() const { return cuts; }
    
     vector<vector<cv::Point>> contours;
     vector<cv::Vec4i> hierarchy;
     vector<Cut> cuts;

private:
    void findContoursAndHierarchy(const cv::Mat &bin);
    void computeOptimalCuts();
    
    Cut findMinDistanceCut(const  vector<cv::Point>& contour1, int idx1,
                          const  vector<cv::Point>& contour2, int idx2);
    
    vector<Cut> buildCutsForContour(int outerIdx, const vector<int>& holes);
    
};

#endif