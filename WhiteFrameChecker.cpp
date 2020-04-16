#include "opencv2/opencv.hpp"
#include <iostream>
#include <charconv>
#include "exlib\Utils\exstring.h"

using namespace cv;
using namespace std::literals;

std::size_t round_up(std::size_t a, std::size_t b)
{
	return ((a + b - 1) / b) * b;
}

std::size_t round_down(std::size_t a, std::size_t b)
{
	return (a / b) * b;
}

template<typename T>
struct type_identity {
	using type = T;
};
template<typename T>
bool all_same(T const* begin, T const* end, typename type_identity<T>::type value)
{
	for(auto ptr = begin; ptr < end; ++ptr)
	{
		if(*ptr != value)
		{
			return false;
		}
	}
	return true;
}

bool check_all_same_cont(std::uint8_t const* pointer_begin, std::uint8_t const* pointer_end, std::size_t comp_value)
{
	auto const aligned_begin = reinterpret_cast<std::size_t const*>(round_up(reinterpret_cast<std::size_t>(pointer_begin), alignof(std::size_t)));
	auto const aligned_end = reinterpret_cast<std::size_t const*>(round_down(reinterpret_cast<std::size_t>(pointer_end), alignof(std::size_t)));
	auto const aligned_begin_gray = reinterpret_cast<std::uint8_t const*>(aligned_begin);
	auto const aligned_end_gray = reinterpret_cast<std::uint8_t const*>(aligned_end);
	return all_same(pointer_begin, aligned_begin_gray, comp_value) &&
		all_same(aligned_begin, aligned_end, comp_value) &&
		all_same(aligned_end_gray, pointer_end, comp_value);
}

bool all_same_binary(Mat const& gray, bool check_all_black)
{
	auto const comp_value = std::size_t(check_all_black) - std::size_t(1);
	auto const rows = std::size_t(gray.rows);
	auto const y_step = std::size_t(gray.step[0]);
	auto const begin = gray.data;
	if(gray.isContinuous())
	{
		return check_all_same_cont(begin, begin + rows * y_step, comp_value);
	}
	else
	{
		auto const cols = std::size_t(gray.cols);
		auto const x_step = std::size_t(gray.step[1]);
		for(int y = 0; y < rows; ++y)
		{
			auto const row_begin = begin + y * y_step;
			auto const row_end = row_begin + cols * x_step;
			if(!check_all_same_cont(row_begin, row_end, comp_value))
			{
				return false;
			}
		}
		return true;
	}
}

std::string hms_string(double total_seconds)
{
	auto constexpr seconds_in_min = 60.;
	auto constexpr seconds_in_hour = 60. * 60.;
	auto const hours_double = std::floor(total_seconds / seconds_in_hour);
	auto const minutes_double = std::floor((total_seconds - hours_double * seconds_in_hour) / 60.);
	auto const seconds_double = std::fmod(total_seconds, 60);
	auto const hours = std::size_t(hours_double);
	auto const minutes = std::size_t(minutes_double);
	auto const seconds = seconds_double;
	auto hours_part = (hours > 0) ? std::to_string(hours).append(":"sv) : std::string();
	auto minutes_part = ((hours > 0) ? exlib::front_padded_string(std::to_string(minutes), 2, '0') : std::to_string(minutes)).append(":"sv);
	auto seconds_part = std::to_string(seconds);
	{
		auto const decimal_point = std::find(seconds_part.begin(), seconds_part.end(), '.');
		if(auto const distance = decimal_point - seconds_part.begin(); distance < 2)
		{
			auto const count = std::size_t(2 - distance);
			seconds_part.insert(seconds_part.begin(), count, '0');
		}
	}
	return std::move(hours_part) + minutes_part + seconds_part;
}


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cout << "Video file name\n";
		return 1;
	}
	std::uint64_t skip = 0;
	if(argc >= 3)
	{
		std::from_chars(argv[2], reinterpret_cast<char const*>(std::size_t(-1)), skip);
		if(skip > 0)
		{
			std::cout << "Skipping " + std::to_string(skip) + " frames\n";
		}
	}
	VideoCapture cap(argv[1]); // open the default camera
	if(!cap.isOpened())  // check if we succeeded
	{
		std::cout << "Failed to open video\n";
		return 1;
	}
	auto const frame_rate = double(cap.get(CAP_PROP_FPS));
	Mat frame;
	Mat gray;
	for(std::uintmax_t count = 0;; ++count)
	{
		auto const success = cap.grab();
		if(!success)
		{
			std::cout << "No more frames. Parsed " + std::to_string(count) + " frames\n";
			break;
		}
		{
			auto const message = "At frame "s + std::to_string(count) + "\r";
			std::cout << message;
		}
		if(count < skip)
		{
			continue;
		}
		cap.retrieve(frame);
		// cvtColor(frame, gray, COLOR_BGR2GRAY);
		if(all_same_binary(frame, false))
		{
			std::cout << "Frame "s + std::to_string(count) + " at " + hms_string(count / frame_rate) + " is all white.\n";
		}
		/*if(all_same_binary(frame, true))
		{
			std::cout << "Frame "s + std::to_string(count) + " at " + hms_string(count / frame_rate) + " is all black.\n";
		}*/
	}
	return 0;
}