#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <opencv\highgui.h>
#include <filesystem>
#include <sys/stat.h>

#include "Frame.h"
#include "Face.h"
#include "ImageListener.h"
#include "FrameDetector.h"
#include "VideoDetector.h"
#include "AffdexException.h"

using namespace std;
using namespace affdex;

const string LOG_FILE = "log.csv";
const string VIDEO_PATH = "video.avi";

float last_timestamp = -1.0f;
float capture_fps = -1.0f;
float process_last_timestamp = -1.0f;
float process_fps = -1.0f;

class PlottingImageListener : public ImageListener
{
public:
	void onImageResults(std::map<FaceId, Face> faces, Frame image) {

		const int spacing = 10;
		const int left_margin = 30;
		const int font = cv::FONT_HERSHEY_COMPLEX_SMALL;
		float font_size = 0.5f;
		cv::Scalar clr = cv::Scalar(0, 0, 255);
		cv::Scalar header_clr = cv::Scalar(255, 0, 0);
		shared_ptr<byte> imgdata = image.getBGRByteArray();
		cv::Mat mImg = cv::Mat(image.getHeight(), image.getWidth(), CV_8UC3, imgdata.get());

		//Process only one face
		if (faces.size() == 1)
		{
			Face f = faces[0];
			std::ofstream out;
			out.open(LOG_FILE, std::ios_base::app);
			VecFeaturePoint points = f.featurePoints;
			for (auto& point : points)	//Draw face feature points.
			{
				cv::circle(mImg, cv::Point(point.x, point.y), 1.0f, cv::Scalar(0, 0, 255));
			}
			Orientation headAngles = f.measurements.orientation;
			std::string strAngles = "Pitch: " + std::to_string(headAngles.pitch) +
				" Yaw: " + std::to_string(headAngles.yaw) +
				" Roll: " + std::to_string(headAngles.roll) +
				" InterOcularDist: " + std::to_string(f.measurements.interocularDistance);

			//Output the results of the different classifiers.
			int padding = 10;

			std::vector<std::string> expressions{ "smile", "innerBrowRaise", "browRaise", "browFurrow", "noseWrinkle",
				"upperLipRaise", "lipCornerDepressor", "chinRaise", "lipPucker", "lipPress",
				"lipSuck", "mouthOpen", "smirk", "eyeClosure", "attention" };

			std::vector<std::string> emotions{ "joy", "fear", "disgust", "sadness", "anger", "surprise", "contempt", "valence", "engagement" };

			cv::putText(mImg, "MEASUREMENTS", cv::Point(left_margin, padding += spacing), font, font_size, header_clr);

			cv::putText(mImg, strAngles, cv::Point(left_margin, padding += spacing), font, font_size, clr);

			cv::putText(mImg, "EXPRESSIONS", cv::Point(left_margin, padding += (spacing * 2)), font, font_size, header_clr);
			float * values = (float *)&f.expressions;
			for (string expression : expressions)
			{
				out << std::to_string(int(*values)) + ";";
				cv::putText(mImg, expression + ": " + std::to_string(int(*values)), cv::Point(left_margin, padding += spacing), font, font_size, clr);
				values++;
			}
			cv::putText(mImg, "EMOTIONS", cv::Point(left_margin, padding += (spacing * 2)), font, font_size, header_clr);

			values = (float *)&f.emotions;
			for (string emotion : emotions)
			{
				out << std::to_string(int(*values)) + ";";
				cv::putText(mImg, emotion + ": " + std::to_string(int(*values)), cv::Point(left_margin, padding += spacing), font, font_size, clr);
				values++;
			}
			std::cerr << "Timestamp: " << image.getTimestamp()
				<< "," << image.getWidth()
				<< "x" << image.getHeight()
				<< " cfps: " << capture_fps
				<< " pnts: " << points.size() << endl;
			process_last_timestamp = image.getTimestamp();
			out << process_last_timestamp << endl;
		}
		cv::putText(mImg, "capture fps: " + std::to_string(int(capture_fps)), cv::Point(mImg.cols - 110, mImg.rows - left_margin - spacing), font, font_size, clr);

		cv::imshow("analyze-image", mImg);

		cv::waitKey(30);
	};


	void onImageCapture(Frame image) override
	{};
};

int video(string videoPath)
{
	try{
		// Parse and check the data folder (with assets)
		const std::wstring AFFDEX_DATA_DIR = L"C:\\Program Files (x86)\\Affectiva\\Affdex SDK\\data";
		const std::wstring AFFDEX_LICENSE_FILE = L"C:\\Program Files (x86)\\Affectiva\\Affdex SDK\\data\\affdex.license";
		std::wstring v(videoPath.begin(), videoPath.end());
		int framerate = 30;

		VideoDetector videoDetector(framerate);
		shared_ptr<ImageListener> listenPtr(new PlottingImageListener());	// Instanciate the ImageListener class
		auto start_time = std::chrono::system_clock::now();

		struct stat buffer;
		if (!stat(videoPath.c_str(), &buffer) == 0)
		{
			std::cerr << "No video found!" << std::endl;
			return 1;
		}

		//Initialize detectors
		videoDetector.setDetectAllEmotions(true);
		videoDetector.setDetectAllExpressions(true);
		videoDetector.setClassifierPath(AFFDEX_DATA_DIR);
		videoDetector.setLicensePath(AFFDEX_LICENSE_FILE);
		videoDetector.setImageListener(listenPtr.get());
		//Start the frame detector thread.
		videoDetector.start();

		//Calculate the Image timestamp and the capture frame rate;
		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time);
		const float seconds = milliseconds.count() / 1000.f;
		// Create a frame
		capture_fps = 1.0f / (seconds - last_timestamp);
		last_timestamp = seconds;
		std::cerr << "Start process." << std::endl;
		videoDetector.process(v);  //Pass the video to detector

		while (videoDetector.isRunning())
		{
			//wait for the end of processing
		}
		videoDetector.stop();	//Stop frame detector thread
	}
	catch (AffdexException ex)
	{
		std::cerr << "Encountered an AffdexException " << ex.what();
		return 1;
	}
	catch (std::exception ex)
	{
		std::cerr << "Encountered an exception " << ex.what();
		return 1;
	}
	catch (std::runtime_error err)
	{
		std::cerr << "Encountered a runtime error " << err.what();
		return 1;
	}

	return 0;
}

int webcam(void)
{
	try{
		// Parse and check the data folder (with assets)
		const std::wstring AFFDEX_DATA_DIR = L"C:\\Program Files (x86)\\Affectiva\\Affdex SDK\\data";
		const std::wstring AFFDEX_LICENSE_FILE = L"C:\\Program Files (x86)\\Affectiva\\Affdex SDK\\data\\affdex.license";

		int framerate = 30;
		int process_frame_rate = 30;
		int buffer_length = 2;

		FrameDetector frameDetector(buffer_length, process_frame_rate);		// Init the FrameDetector Class
		shared_ptr<ImageListener> listenPtr(new PlottingImageListener());	// Instanciate the ImageListener class

		cv::VideoCapture webcam(0);	//Connect to the first webcam
		webcam.set(CV_CAP_PROP_FPS, framerate);	//Set webcam framerate.
		std::cerr << "Setting the webcam frame rate to: " << framerate << std::endl;
		auto start_time = std::chrono::system_clock::now();
		if (!webcam.isOpened())
		{
			std::cerr << "Error opening webcam!" << std::endl;
			return 1;
		}

		//Initialize detectors
		frameDetector.setDetectAllEmotions(true);
		frameDetector.setDetectAllExpressions(true);
		frameDetector.setClassifierPath(AFFDEX_DATA_DIR);
		frameDetector.setLicensePath(AFFDEX_LICENSE_FILE);
		frameDetector.setImageListener(listenPtr.get());
		//Start the frame detector thread.
		frameDetector.start();

		do{
			cv::Mat img;

			if (!webcam.read(img))	//Capture an image from the camera
			{
				std::cerr << "Failed to read frame from webcam! " << std::endl;
				break;
			}

			//Calculate the Image timestamp and the capture frame rate;
			const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time);
			const float seconds = milliseconds.count() / 1000.f;
			// Create a frame
			Frame f(img.size().width, img.size().height, img.data, Frame::COLOR_FORMAT::BGR, seconds);
			capture_fps = 1.0f / (seconds - last_timestamp);
			last_timestamp = seconds;
			std::cerr << "Capture framerate = " << capture_fps << std::endl;
			frameDetector.process(f);  //Pass the frame to detector
		} while (!GetAsyncKeyState(VK_ESCAPE));

		frameDetector.stop();	//Stop frame detector thread
	}
	catch (AffdexException ex)
	{
		std::cerr << "Encountered an AffdexException " << ex.what();
		return 1;
	}
	catch (std::exception ex)
	{
		std::cerr << "Encountered an exception " << ex.what();
		return 1;
	}
	catch (std::runtime_error err)
	{
		std::cerr << "Encountered a runtime error " << err.what();
		return 1;
	}

	return 0;
}

int main(int argsc, char ** argsv)
{
	std::ofstream out(LOG_FILE);
	out << "smile;innerBrowRaise;browRaise;browFurrow;noseWrinkle;upperLipRaise;lipCornerDepressor;chinRaise;lipPucker;lipPress;lipSuck;mouthOpen;smirk;eyeClosure;attention;";
	out << "joy;fear;disgust;sadness;anger;surprise;contempt;valence;engagement;time" << endl;
	out.close();
	//webcam();
	video(VIDEO_PATH);
}