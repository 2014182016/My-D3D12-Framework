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
�ּ��� �ε��ϴ� Ŭ����
�޽�, ���� ���� �����͸� �����ӿ�ũ���� ����� �� �ֵ��� ��ȯ�Ѵ�.
*/
class AssetLoader
{
public:
	// Ȯ���� h3d������ �ε��Ͽ� ���� �� �ε����� ��ȯ�Ѵ�.
	static bool LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::wstring fileName,
		std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices);
	// obj�� h3d���Ϸ� ��ȯ�Ѵ�.
	static bool ConvertObj(const std::wstring fileName);
	// wav������ ����Ʈ�Ͽ� ���� �����Ϳ� ��� ����ü�� ��ȯ�Ѵ�.
	static FILE* LoadWave(const std::string fileName, WaveHeaderType& header);

	// obj�� h3d�� ��ȯ�Ѵ�.
	static void ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices, std::wstring fileName);
	// �޽��� �� ������ �̿��Ͽ� TBN�� ����ϰ� �� normal, tangent, binormal�� ��ȯ�Ѵ�.
	static void CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
};