/**
 *
 * @file: WubiCode.h
 * @desc: ������ͱ����ѯ
 * @auth: liwei (www.leewei.org)
 * @mail: ari.feng@qq.com
 * @date: 2012/06/10
 * @mdfy: 2012/06/10
 * @Ver:  1.0
 *
 */

#ifndef _WUBI_CODE_H
#define _WUBI_CODE_H

#include <string>

class CWubiCode
{
public:
	CWubiCode(void);
	~CWubiCode(void);

	static std::string			Find(std::wstring ch);			//����һ�����ֵ���ʱ���
};

#endif