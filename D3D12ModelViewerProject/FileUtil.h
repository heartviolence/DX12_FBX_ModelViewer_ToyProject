#pragma once

#include <filesystem>

class FileUtil
{
public:
	// fullfilepath를 넘겨주면 디렉토리경로만 반환함
	static std::wstring GetDirectory(std::wstring filePath);

	// fullfilepath를 넘겨주면 확장자없는 이름만 반환함
	static std::wstring GetFileNameWithoutExtension(std::wstring filePath);

	// fullfilepath를 넘겨주면 파일 존재여부를 반환
	static bool IsFileExist(std::wstring filePath);

	// fullfilepath를 넘겨주면 확장자 반환  (.fbx , .exe)
	static std::wstring GetFileExtension(std::wstring filePath);

	//상대경로와 디렉토리로 절대경로를 반환
	static std::wstring GetAbsoluteFilePath(std::wstring directory, std::wstring relativeName);
};


