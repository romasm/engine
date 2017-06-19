#pragma once

#include "stdafx.h"
#include "Log.h"
#include "DataTypes.h"

using namespace EngineCore;

// Строковые функции

inline wstring StringToWstring( const string& s )
{
	int len;
    int slength = (int)s.length();
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
};

inline string WstringToString( const wstring& s )
{
	int len;
    int slength = (int)s.length();
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
    std::string r(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0); 
    return r;
}

inline wchar_t* CharToWChar(char *mbString)
{ 
	size_t len = 0; 
	size_t t;
	len = (int)strlen(mbString) + 1; 
	wchar_t *ucString = new wchar_t[len]; 
	mbstowcs_s(&t, ucString, len, mbString, len); 
	return ucString; 
}


inline char* WCharToChar(wchar_t *wcstring)
{ 
	size_t len = 0; 
	size_t t;
	len = (size_t)wcslen(wcstring) + 1; 
	char *mbstring = new char[len]; 
	wcstombs_s(&t, mbstring,len,wcstring,len); 
	return mbstring; 
}

inline std::string IntToString(int i)
{
	char str[SRT_SIZE];
	sprintf_s(str, "%i", i);
	return string(str);  
}

inline std::string FloatToString(float f)
{
	char str[SRT_SIZE];
	sprintf_s(str, SRT_SIZE, "%f", f);
	return string(str); 
}

inline float CharToFloat(char *str)
{
	float res = 0.0f;
	int minus = 1;
	bool dot = false;
	int afterdot = 0;

	int i=0;
	while((*(str+i)>'9' || *(str+i)<'0') && *(str+i)!='.' && *(str+i)!=',' && i<SRT_SIZE)
	{
		if(*(str+i)=='-')
			minus = -1;
		i++;
	}

	while((*(str+i)<='9' && *(str+i)>='0') || *(str+i)=='.' || *(str+i)==',' && i<SRT_SIZE)
	{
		if((*(str+i)=='.' || *(str+i)==',') && !dot)
		{
			dot = true;
			i++;
			continue;
		}
		else
			if(*(str+i)=='.' || *(str+i)==',')return res*minus;

		if(!dot)
		{
			res = res*10.0f;
			res += float(int(*(str+i)-'0'));
		}
		else
		{
			afterdot++;
			res += float(int(*(str+i)-'0'))/pow(10.0f,afterdot);
		}

		i++;
	}

	return res*minus;
}

inline void FloatToChar(char* str, float f)
{
	if(!str)return;
	sprintf_s(str, SRT_SIZE, "%f", f); 
}

inline XMFLOAT3 CharToXMFloat3(char *str)
{
	XMFLOAT3 res = XMFLOAT3(0.0f,0.0f,0.0f);
	
	char temp[128];
	strcpy_s(temp, 128, str);

	int i=0;
	while(temp[i]!=' ' && temp[i]!='\n' && temp[i]!='\t' && temp[i]!='\0' && temp[i]!='"' && temp[i]!='\'' && i<SRT_SIZE )
	{
		i++;
	}
	temp[i] = 0;
	res.x = CharToFloat(temp);
	if(*(str+i)==0)return res;
	i++;

	strcpy_s(temp, 128, (str+i));
	int j=0;
	while(temp[j]!=' ' && temp[j]!='\n' && temp[j]!='\t' && temp[j]!='\0' && temp[j]!='"' && temp[j]!='\'' && j<SRT_SIZE )
	{
		j++;
	}
	temp[j] = 0;
	res.y = CharToFloat(temp);
	if(*(str+i+j)==0)return res;
	j++;

	strcpy_s(temp, 128, (str+i+j));
	int k=0;
	while(temp[k]!=' ' && temp[k]!='\n' && temp[k]!='\t' && temp[k]!='\0' && temp[k]!='"' && temp[k]!='\'' && k<SRT_SIZE )
	{
		k++;
	}
	temp[k] = 0;
	res.z = CharToFloat(temp);

	return res;
}

inline void XMFloat3ToChar(char* str, XMFLOAT3 f)
{
	if(!str)return;
	sprintf_s(str, SRT_SIZE, "%f %f %f", f.x, f.y, f.z); 
}

inline string XMFloat3ToString(XMFLOAT3 f)
{
	char str[SRT_SIZE];
	sprintf_s(str, SRT_SIZE, "%f %f %f", f.x, f.y, f.z); 
	return string(str);  
}

inline XMFLOAT4 CharToXMFloat4(char *str)
{
	XMFLOAT4 res = XMFLOAT4(0.0f,0.0f,0.0f,0.0f);
	
	char temp[128];
	strcpy_s(temp, 128, str);

	int i=0;
	while(temp[i]!=' ' && temp[i]!='\n' && temp[i]!='\t' && temp[i]!='\0' && temp[i]!='"' && temp[i]!='\'' && i<SRT_SIZE )
		i++;
	temp[i] = 0;
	res.x = CharToFloat(temp);
	if(*(str+i)==0)return res;
	i++;

	strcpy_s(temp, 128, (str+i));
	int j=0;
	while(temp[j]!=' ' && temp[j]!='\n' && temp[j]!='\t' && temp[j]!='\0' && temp[j]!='"' && temp[j]!='\'' && j<SRT_SIZE )
		j++;
	temp[j] = 0;
	res.y = CharToFloat(temp);
	if(*(str+i+j)==0)return res;
	j++;

	strcpy_s(temp, 128, (str+i+j));
	int h=0;
	while(temp[h]!=' ' && temp[h]!='\n' && temp[h]!='\t' && temp[h]!='\0' && temp[h]!='"' && temp[h]!='\'' && h<SRT_SIZE )
		h++;
	temp[h] = 0;
	res.z = CharToFloat(temp);
	if(*(str+i+j+h)==0)return res;
	h++;

	strcpy_s(temp, 128, (str+i+j+h));
	int k=0;
	while(temp[k]!=' ' && temp[k]!='\n' && temp[k]!='\t' && temp[k]!='\0' && temp[k]!='"' && temp[k]!='\'' && k<SRT_SIZE )
	{
		k++;
	}
	temp[k] = 0;
	res.w = CharToFloat(temp);

	return res;
}

inline void XMFloat4ToChar(char* str, XMFLOAT4 f)
{
	if(!str)return;
	sprintf_s(str, SRT_SIZE, "%f %f %f %f", f.x, f.y, f.z, f.w); 
}

inline string XMFloat4ToString(XMFLOAT4 f)
{
	char str[SRT_SIZE];
	sprintf_s(str, SRT_SIZE, "%f %f %f %f", f.x, f.y, f.z, f.w); 
	return string(str);  
}

inline unsigned char CharToByte(char *str)
{
	unsigned char res = 0;
	int i=0;
	int j=0;

	if(str[i]=='0' && str[i+1]=='b')
		i += 2;

	while(str[i]!='\n' && str[i]!='\0' && str[i]!='"' && str[i]!='\'' && j<8 && i<SRT_SIZE)
	{
		if(str[i] == '1')
		{
			res = res | unsigned char(pow(2, 7-j));
			j++;
		}else if(str[i] == '0')
			j++;
		i++;
	}

	return res;
}

inline void ByteToChar(char* str, unsigned char f)
{
	if(!str)return;
	int output[8];

	for(int i=0; i<8; i++)
	{
		if( (f & unsigned char(pow(2, i))) != 0 )
			output[i] = 1;
		else
			output[i] = 0;
	}

	sprintf_s(str, SRT_SIZE, "0b%i%i%i%i%i%i%i%i", output[7], output[6], output[5], output[4], output[3], output[2], output[1], output[0]); 
}

inline string ByteToString(uint8_t b)
{
	char str[SRT_SIZE];
	ByteToChar(str, b);
	return string(str);  
}

inline int CharToInt(char *str)
{
	return atoi(str);
}

inline int WCharToInt(wchar_t *str)
{
	return _wtoi(str);
}

inline bool CharToBool(char *str)
{
	if( strcmp(str, "true") == 0 || strcmp(str, "TRUE") == 0 )return true;
	else return false;
}

inline string BoolToString(bool b)
{
	char str[SRT_SIZE];
	if(b)
		sprintf_s(str, SRT_SIZE, "true");
	else
		sprintf_s(str, SRT_SIZE, "false");
	return string(str);  
}

inline char* CharToChar(char *str)
{
	char* res = new char[SRT_SIZE];
	strcpy_s(res, SRT_SIZE, str);
	return res;
}

inline wchar_t* WCharToWChar(wchar_t *str)
{
	wchar_t* res = new wchar_t[SRT_SIZE];
	wcscpy_s(res, SRT_SIZE, str);
	return res;
}

// Математические функции
inline XMFLOAT4 XMFloat4Lerp(XMFLOAT4 f1, XMFLOAT4 f2, float l)
{
	XMFLOAT4 res; 
	res.x = f1.x * (1 - l) + f2.x * l;
	res.y = f1.y * (1 - l) + f2.y * l;
	res.z = f1.z * (1 - l) + f2.z * l;
	res.w = f1.w * (1 - l) + f2.w * l;
	return res;
}

inline XMFLOAT3 XMFloat3Lerp(XMFLOAT3 f1, XMFLOAT3 f2, float l)
{
	XMFLOAT3 res; 
	res.x = f1.x * (1 - l) + f2.x * l;
	res.y = f1.y * (1 - l) + f2.y * l;
	res.z = f1.z * (1 - l) + f2.z * l;
	return res;
}

inline XMFLOAT2 XMFloat2Lerp(XMFLOAT2 f1, XMFLOAT2 f2, float l)
{
	XMFLOAT2 res; 
	res.x = f1.x * (1 - l) + f2.x * l;
	res.y = f1.y * (1 - l) + f2.y * l;
	return res;
}

inline float lerp(float f1, float f2, float l)
{
	return f1 * (1 - l) + f2 * l;
}

inline float clamp(float fmin, float f, float fmax)
{
	return max(fmin, min(f, fmax));
}

inline int FloatRoundInt(float f)
{
	return int(f < 0.0f ? ceil(f - 0.5f) : floor(f + 0.5f));
}

inline MLRECT RECTtoMLRECT(RECT r)
{
	MLRECT res;
	res.top = r.top;
	res.left = r.left;
	res.height = r.bottom - r.top;
	res.width = r.right - r.left;
	return res;
}

inline RECT MLRECTtoRECT(MLRECT r)
{
	RECT res;
	res.top = r.top;
	res.left = r.left;
	res.bottom = r.top + r.height;
	res.right = r.left + r.width;
	return res;
}

inline bool isInRect(RECT child, RECT parent)
{
	if(child.right < parent.left || child.bottom < parent.top || 
		child.left > parent.right || child.top > parent.bottom)
		return false;
	else
		return true;
}

inline POINT ClampPoint(POINT x, POINT xmin, POINT xmax)
{
	POINT r;
	r.x = min(x.x, xmax.x);
	r.x = max(r.x, xmin.x);
	r.y = min(x.y, xmax.y);
	r.y = max(r.y, xmin.y);
	return r;
}

//Расширение для vector
template<typename E>
static bool isIn(vector<E>& v, E d)
{
	for(int i=0; i<(int)v.size(); i++)
		if(d == v.[i])
			return true;
	return false;
};