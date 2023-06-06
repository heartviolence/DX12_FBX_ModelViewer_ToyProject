#pragma once

#include <filesystem>

class FileUtil
{
public:
	// fullfilepath�� �Ѱ��ָ� ���丮��θ� ��ȯ��
	static std::wstring GetDirectory(std::wstring filePath);

	// fullfilepath�� �Ѱ��ָ� Ȯ���ھ��� �̸��� ��ȯ��
	static std::wstring GetFileNameWithoutExtension(std::wstring filePath);

	// fullfilepath�� �Ѱ��ָ� ���� ���翩�θ� ��ȯ
	static bool IsFileExist(std::wstring filePath);

	// fullfilepath�� �Ѱ��ָ� Ȯ���� ��ȯ  (.fbx , .exe)
	static std::wstring GetFileExtension(std::wstring filePath);

	//����ο� ���丮�� �����θ� ��ȯ
	static std::wstring GetAbsoluteFilePath(std::wstring directory, std::wstring relativeName);
};


