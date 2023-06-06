
#include "FileUtil.h"

std::wstring FileUtil::GetDirectory(std::wstring filePath)
{
	namespace fs = std::filesystem;

	fs::path directory(filePath);
	std::wstring result(directory.remove_filename());

	return result;
}


std::wstring FileUtil::GetFileNameWithoutExtension(std::wstring filePath)
{
	std::filesystem::path path(filePath);

	return std::wstring(path.replace_extension("").filename());
}

bool FileUtil::IsFileExist(std::wstring filePath)
{
	bool result = false;
	if (std::filesystem::is_directory(filePath) == false)
	{
		result = std::filesystem::exists(filePath);
	}
	return result;
}

std::wstring FileUtil::GetFileExtension(std::wstring filePath)
{
	std::filesystem::path path(filePath);

	return std::wstring(path.extension());
}


std::wstring FileUtil::GetAbsoluteFilePath(std::wstring directory, std::wstring relativePath)
{
	namespace fs = std::filesystem;

	return std::wstring(fs::weakly_canonical(fs::path(directory).append(relativePath))); ;
}