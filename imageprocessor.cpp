#include "imageprocessor.h"
#include <limits>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <opencv2/flann.hpp>

using namespace std;

void ImageProcessor::process(const cv::Mat &binImage) {

    if (binImage.empty() || binImage.type() != CV_8UC1) {
        cerr << "[ImageProcessor] process: входное изображение пустое или не CV_8UC1\n";
        return;
    }

    cv::Mat bin;
    cv::threshold(binImage, bin, 127, 255, cv::THRESH_BINARY);

    findContoursAndHierarchy(bin);
    computeOptimalCuts();
}

void ImageProcessor::findContoursAndHierarchy(const cv::Mat &bin) {
    cv::Mat tmp = bin.clone();
    vector<vector<cv::Point>> rawContours;
    vector<cv::Vec4i> hier;

    cv::findContours(tmp, rawContours, hier, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
    contours = move(rawContours);
    hierarchy = move(hier);
}



void ImageProcessor::computeOptimalCuts() {
    cuts.clear();
    for (size_t i = 0; i < contours.size(); ++i) {
        if (contours[i].empty()) continue;

        double signedArea = cv::contourArea(contours[i], true);
        if (signedArea <= 0) continue;

        vector<int> holeIndices;
        int firstChild = hierarchy[i][2];

        if (firstChild != -1) {
            int currentHole = firstChild;
            while (currentHole != -1) {
                if (!contours[currentHole].empty() && 
                    cv::contourArea(contours[currentHole], true) < 0) {
                    holeIndices.push_back(currentHole);
                }
                currentHole = hierarchy[currentHole][0];
            }

            if (!holeIndices.empty()) {
                vector<Cut> contourCuts = buildCutsForContour((int)i, holeIndices);
                for (const Cut& cut : contourCuts) {
                    cuts.push_back(cut);
                }
            }
        }
    }
}

vector<Cut> ImageProcessor::buildCutsForContour(int outerIdx, const vector<int>& holes) {
    vector<Cut> result;
    result.reserve(holes.size());
    
    for (int holeIdx : holes) {
        if (!contours[outerIdx].empty() && !contours[holeIdx].empty()) {
            result.push_back(findMinDistanceCut(contours[outerIdx], outerIdx,
                                                contours[holeIdx], holeIdx));
        }
    }
    
    return result;
}

Cut ImageProcessor::findMinDistanceCut(const vector<cv::Point>& outer, int idxOuter,
                                       const vector<cv::Point>& inner, int idxInner) {
    Cut bestCut;
    bestCut.contour_out = idxOuter;
    bestCut.contour_in = idxInner;
    double minDist = numeric_limits<double>::max();
    
    if (outer.empty() || inner.empty()) return bestCut;

    cv ::Mat points(outer.size(), 2, CV_32F);
    for (size_t i = 0; i < outer.size(); ++i) {
        points.at<float>(i, 0) = outer[i].x;
        points.at<float>(i, 1) = outer[i].y;
    }
    
    cv::flann::Index kdTree(points, cv::flann::KDTreeIndexParams(2));

    for (const auto& pt : inner) {
        cv::Mat query = (cv::Mat_<float>(1, 2) << pt.x, pt.y);
        vector<int> indices(1);
        vector<float> dists(1);
        
        kdTree.knnSearch(query, indices, dists, 1);
        
        if (dists[0] < minDist) {
            minDist = dists[0];
            bestCut.p_out = outer[indices[0]];
            bestCut.p_in = pt;
            }
        }
    
    
    return bestCut;
}

vector<vector<cv::Point2f>> ImageProcessor::mergedContours(){
    vector<vector<cv::Point2f>> result;
    
    for (size_t i = 0; i < contours.size(); ++i) {
        if (contours[i].empty() || cv::contourArea(contours[i], true) <= 0) {
            continue;
        }
        vector<cv::Point2f> merged;
        for (const auto& point : contours[i]) {
            merged.push_back(cv::Point2f(point.x, point.y));
        }
        for (const auto& cut : cuts) {
            if (cut.contour_out == static_cast<int>(i)) {
                merged.push_back(cut.p_out);
                merged.push_back(cut.p_in);
                
                const auto& hole = contours[cut.contour_in];
                for (const auto& point : hole) {
                    merged.push_back(cv::Point2f(point.x, point.y));
                }
                merged.push_back(cut.p_in);
                merged.push_back(cut.p_out);
            }
        }
        
        result.push_back(move(merged));
    }
    
    return result;
}

float ImageProcessor::totalCutsLength() const {
    float sum = 0.0f;
    for (const auto& cut : cuts) 
        sum += cut.length();
    return sum;
}

string ImageProcessor::getInfoString() const {
    stringstream ss;
    ss << "Найдено контуров: " << contours.size() << "\n";

    int outer = 0;
    for (const auto& h : hierarchy) {
        if (h[3] == -1) 
            outer++;
    }
    int inner = hierarchy.size() - outer;

    ss << "Внешние контуры: " << outer << "\n";
    ss << "Отверстия: " << inner << "\n";
    ss << "Сделано разрезов: " << cuts.size() << "\n";
    ss << "Общая длина разрезов: " << totalCutsLength() << "\n";

    return ss.str();
}
