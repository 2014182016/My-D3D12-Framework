#include <Framework/AssetLoader.h>
#include <string>
#include <iostream>

static const std::wstring modelPath = L"../Assets/Models/";

int main()
{
	const std::wstring fileName = L"Skull.obj";
	const std::wstring filePath = modelPath + fileName;

	bool result = AssetLoader::ConvertObj(filePath);
	std::cout << "Result : " << std::boolalpha << result << std::endl;

	return 0;
}