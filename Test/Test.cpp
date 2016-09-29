// Test.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"

static HRESULT enumMFTransforms();

int main()
{
	enumMFTransforms();

    return 0;
}

HRESULT enumMFTransforms()
{
	CLSID* pClsId;
	UINT32 count;
	HR_ASSERT_OK(MFTEnum(GUID_NULL, 0, NULL, NULL, NULL, &pClsId, &count));

	std::cout << "Count=" << count << std::endl;

	return S_OK;
}
