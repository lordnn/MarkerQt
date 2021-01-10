/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "worker.h"
#include <lbf/lbf.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

#include <type_traits>

namespace dlib {
    struct bgr_alpha_pixel
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a simple struct that represents an BGR colored graphical pixel
                with an alpha channel.
        !*/

        bgr_alpha_pixel (
        ) {}

        bgr_alpha_pixel (
            unsigned char blue_,
            unsigned char green_,
            unsigned char red_,
            unsigned char alpha_
        ) : blue(blue_), green(green_), red(red_), alpha(alpha_) {}

        unsigned char blue;
        unsigned char green;
        unsigned char red;
        unsigned char alpha;
    };

    template <>
    struct pixel_traits<bgr_alpha_pixel>
    {
        const static bool rgb  = false;
        const static bool rgb_alpha  = true;
        const static bool grayscale = false;
        const static bool hsi = false;
        const static bool lab = false;
        enum {num = 4};
        typedef unsigned char basic_pixel_type;
        static basic_pixel_type min() { return 0;}
        static basic_pixel_type max() { return 255;}
        const static bool is_unsigned = true;
        const static bool has_alpha = true;
    };

    template <>
    struct image_traits<QImage>
    {
        typedef dlib::bgr_alpha_pixel pixel_type;
    };

    const void* image_data(const QImage &img) {
        return img.bits();
    }
    long width_step(const QImage &img) {
        return img.bytesPerLine();
    }
    long num_rows(const QImage &img) {
        return img.height();
    }
    long num_columns(const QImage &img) {
        return img.width();
    }
} // namespace dlib

TWorker::TWorker(workerType type) : _type(type)
{ }

void TWorker::setData(const QImage &img)
{
    _image = img;
}

void TWorker::setRect(const QRect &rect)
{
    _rect = rect;
}

void TWorker::process()
{
    switch (_type) {
    case workerType::wtFaceDetector:
        {
            dlib::array2d<dlib::rgb_pixel> img;
            dlib::assign_image(img, _image);
            dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
            std::vector<dlib::rectangle> dets = detector(img);
            CRectArray frects;
            if (!dets.empty()) {
                std::transform(dets.cbegin(), dets.cend(), std::back_inserter(frects), [](const auto &e){ return QRect(e.left(), e.top(), e.width(), e. height()); });
            }
            emit completeFaceDetector(frects);
        }
        break;
    case workerType::wtLBFRDetector:
#if 0
        {
            std::vector<dlib::rectangle> dets;
            if (_rect.isEmpty()) {
                std::cout << "Rect isEmpty" << std::endl;
                dlib::array2d<dlib::rgb_pixel> img;
                dlib::assign_image(img, _image);
                dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
                dets = detector(img);
            }
            else {
                dets.push_back(dlib::rectangle(_rect.left(), _rect.top(), _rect.right(), _rect.bottom()));
            }
            if (!dets.empty()) {
                auto imgGr = _image.convertToFormat(QImage::Format_Grayscale8);
                lbf::LbfCascador lbfr;
                std::unique_ptr<std::FILE, decltype(&std::fclose)> fp(std::fopen("ibug_helen_dlib_full_model", "rb"), &std::fclose);
                lbfr.Read(fp.get());
                cv::Mat mcopy = cv::Mat(imgGr.height(), imgGr.width(), CV_8UC1, imgGr.bits(), imgGr.bytesPerLine()).clone();
                //cv::cvtColor(mcopy, mcopy, CV_BGRA2Gray);
                lbf::BBox bbox(dets[0].left(), dets[0].top(), dets[0].width(), dets[0].height());
                auto res = lbfr.Predict(mcopy, bbox);
                std::vector<QPointF> pts;
                pts.reserve(res.rows);
                for (int y(0); y < res.rows; ++y) {
                    pts.emplace_back(res.at<double>(y, 0), res.at<double>(y, 1));
                }
                emit completeLBFRDetector(pts);
            }
        }
#else
        {
            std::vector<dlib::rectangle> dets;
            dlib::array2d<dlib::rgb_pixel> img;
            dlib::assign_image(img, _image);
            if (_rect.isEmpty()) {
                std::cout << "Rect isEmpty" << std::endl;
                dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
                dets = detector(img);
            }
            else {
                dets.push_back(dlib::rectangle(_rect.left(), _rect.top(), _rect.right(), _rect.bottom()));
            }
            if (!dets.empty()) {
                static const dlib::shape_predictor sp = []{
                    dlib::shape_predictor tmp;
                    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> tmp;
                    return tmp;
                }();
                //static dlib::shape_predictor sp;
                //static bool bFirst = true;
                //if (bFirst) {
                //    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> sp;
                //    bFirst = false;
                //}
                dlib::full_object_detection shape = sp(img, dets[0]);
                std::vector<QPointF> pts;
                const auto sz{shape.num_parts()};
                pts.reserve(sz);
                for (std::remove_const_t<decltype(sz)> i{}; i < sz; ++i) {
                    const auto &pt = shape.part(i);
                    pts.emplace_back(pt.x(), pt.y());
                }
                emit completeLBFRDetector(pts);
            }
        }
#endif
        break;
    };
    emit finished();
}
