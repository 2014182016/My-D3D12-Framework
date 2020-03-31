/***************************************************************************
풀스크린 버텍스 쉐이더를 사용함으로써 화면 전체의 픽셀에 대하여
픽셀 쉐이더를 실행한다. 버텍스 쉐이더는 풀스크린 버텍스 쉐이더를
사용하고, 이외의 적용하고 싶은 픽셀 쉐이더를 파이프라인 상태 객체에
묶는다. 단, 이 버텍스 쉐이더를 사용하기 위해선 다음과 같은 코드를 실행한다.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);
****************************************************************************/

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float2 texC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	vout.texC = gTexCoords[vid];

	// 스크린 좌표계에서 동차 좌표계로 변환한다.
	vout.posH = float4(2.0f * vout.texC.x - 1.0f, 1.0f - 2.0f * vout.texC.y, 0.0f, 1.0f);

	return vout;
}