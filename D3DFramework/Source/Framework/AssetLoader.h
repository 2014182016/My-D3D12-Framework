#pragma once

#include <Framework/D3DUtil.h>
#include <vector>

struct Vertex;
struct VertexBasic;
enum class CollisionType;
enum class SoundType;

struct MeshInfo
{
	std::string name;
	std::wstring fileName;
	CollisionType collisionType;
};

struct TextureInfo
{
	std::string name;
	std::wstring fileName;
};

struct SoundInfo
{
	std::string name;
	std::string fileName;
	SoundType soundType;
};

struct WaveHeaderType
{
	char chunkId[4];
	unsigned long chunkSize;
	char format[4];
	char subChunkId[4];
	unsigned long subChunkSize;
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned long sampleRate;
	unsigned long bytesPerSecond;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
	char dataChunkId[4];
	unsigned long dataSize;
};

/*
애셋을 로드하는 클래스
메쉬, 사운드 등의 데이터를 프레임워크에서 사용할 수 있도록 변환한다.
*/
class AssetLoader
{
public:
	// 확장자 h3d파일을 로드하여 정점 및 인덱스를 반환한다.
	static bool LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::wstring fileName,
		std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices);
	// obj를 h3d파일로 변환한다.
	static bool ConvertObj(const std::wstring fileName);
	// wav파일을 임포트하여 파일 포인터와 헤더 구조체를 반환한다.
	static FILE* LoadWave(const std::string fileName, WaveHeaderType& header);

	// obj를 h3d로 변환한다.
	static void ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices, std::wstring fileName);
	// 메쉬의 세 정점을 이용하여 TBN을 계산하고 각 normal, tangent, binormal을 반환한다.
	static void CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
};