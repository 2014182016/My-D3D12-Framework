//=============================================================================
// Ssao.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//=============================================================================

#include "Ssao.hlsl"
#include "Defines.hlsl"
#include "Fullscreen.hlsl"

Texture2D gRandomVecMap : register(t2);
Texture2D gPositionMap : register(t3);

float4 PS(VertexOut pin) :SV_Target
{
	// p : ���� �ֺ��� ���� ����ϰ��� �ϴ� �ȼ��� �ش��ϴ� ��
	// n : p������ ���� ����
	// q : p �ֺ��� ������ ��
	// r : p�� ���� ���ɼ��� �ִ� ������ ������

	float4 pos = gPositionMap.Sample(gsamLinearClamp, pin.texC, 0.0f);

	// Position�� ���� ���� SSAO �� SSR üũ ���θ� �ٷ��.
	if (pos.a <= EPSILON)
		return 1.0f;

	// ���� ��ǥ���� ��ġ�� ���, ���̸� �� �������� ��ȯ�Ѵ�.
	float3 posW = mul(pos, gView).xyz;
	float3 normal = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.texC, 0.0f).xyz);
	normal = mul(normal, (float3x3)gView);
	float depth = gDepthMap.SampleLevel(gsamDepthMap, pin.texC, 0.0f).r;
	depth = NdcDepthToViewDepth(depth);

	// ������ �þ߰��� (x,y,z)�� �籸���Ѵ�.
	// �켱 p = t * posW�� �����ϴ� t�� ���Ѵ�.
	// p.z = t * posW.z
	// t = p.z / posW.z
	float3 p = (depth / posW.z) * posW;

	// ������ ���͸� �����ؼ� [0, 1]�� [-1, 1]�� ����Ѵ�.
	float3 randVec = 2.0f * gRandomVecMap.SampleLevel(gsamLinearWrap, 4.0f * pin.texC, 0.0f).rgb - 1.0f;

	float occlusionSum = 0.0f;

	// n������ �ݱ����� p �ֺ��� �̿� ǥ�������� �����Ѵ�.
	for (int i = 0; i < gSampleCount; ++i)
	{
		// �̸� ������ ��� ������ ���͵��� ���� �����Ǿ� �ִ�.
		// (��, ������ ���͵��� ���� �������� �������� �ʴ�.)
		// �� ������ ���͸� �������� �̵��� �ݻ��Ű�� ����
		// ������ ������ ���͵��� ���������.
		float3 offset = reflect(gOffsetVectors[i].xyz, randVec);

		// ������ ���Ͱ� (p, n)���� ���ǵǴ� ����� ������ ���ϰ� ������
		// ������ �ݴ�� �����´�.
		float flip = sign(dot(offset, normal));

		// p�� �߽����� ���� ������ �̳��� ������ �� q�� �����Ѵ�.
		float3 q = p + flip * gOcclusionRadius * offset;

		// q�� �����ؼ� ���� �ؽ�ó ��ǥ�� ���Ѵ�.
		float4 projQ = mul(float4(q, 1.0f), gProjTex);
		projQ /= projQ.w;

		// �������� q�� ���� ���������� ���� ����� �ȼ��� ���̸� ���Ѵ�.
		// (�̰��� q�� ���̴� �ƴϴ�. q�� �׳� p ��ó�� ������ ���̸�)
		// ����� ��ü�� �ƴ� �� ������ �ִ� ���� �� �ִ�.)
		// ���� ����� ���̴� ���� �ʿ��� �����Ѵ�.
		float rz = gDepthMap.SampleLevel(gsamDepthMap, projQ.xy, 0.0f).r;
		rz = NdcDepthToViewDepth(rz);

		// ������ �þ� ���� ��ġ r = (rx, ry, rz)�� �籸���Ѵ�.
		// r�� q�� ������ �������� �����Ƿ� r = t * q�� �����ϴ� t�� �����Ѵ�.
		// r.z = t * q.z�̹Ƿ� t = r.z / q.z �̴�.
		float3 r = (rz / q.z) * q;

		// r�� p�� ������ �� �����Ѵ�.
		// * ���� dot(normal, normalize(r - p))�� ������ ������ r�� (p, normal)����
		//   ���ǵǴ� ��麸�� �󸶳� �տ� �ִ����� ��Ÿ����. �� �տ� �ִ� ���ϼ���
		//   ������ ����ġ�� �� ũ�� ��´�. �̷��� �ϸ� �ڱ� ������, �� r��
		//   �ü��� ������ ��� (p, normal)�� ���� �� ���� ������ ���� �� ���̶�����
		//   r�� p�� �����ٰ� �߸� �����ϴ� ������ �����ȴ�.
		// * ���󵵴� ���� �� p�� ������ r ������ �Ÿ��� �����Ѵ�. r�� p���� �ʹ� �ָ�
		//   ������ p�� ������ �ʴ� ������ �����Ѵ�.
		float distZ = p.z - r.z;
		float dp = max(dot(normal, normalize(r - p)), 0.0f);
		float occlusion = dp * OcclusionFunction(distZ);

		occlusionSum += occlusion;
	}

	occlusionSum /= gSampleCount;
	float access = 1.0f - occlusionSum;

	// SSAO�� �� �� ������ ȿ���� ������, SSAO�� ���(constrast)�� �����Ѵ�.
	return saturate(pow(access, gSsaoContrast));
}
