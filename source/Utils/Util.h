#pragma once

#include "stdafx.h"
#include "Log.h"
#include "DataTypes.h"

using namespace EngineCore;

// Строковые функции

inline void StringSerialize(const string& str, uint8_t** dataPtr, uint32_t* size)
{
	uint32_t str_size = (uint32_t)str.size();
	*(uint32_t*)(*dataPtr) = str_size;
	(*dataPtr) += sizeof(uint32_t);
	(*size) += sizeof(uint32_t);

	memcpy_s((*dataPtr), str_size, str.data(), str_size);
	(*dataPtr) += str_size * sizeof(char);
	(*size) += str_size * sizeof(char);
}

inline string StringDeserialize(uint8_t** dataPtr)
{
	uint32_t str_size = *(uint32_t*)(*dataPtr);
	(*dataPtr) += sizeof(uint32_t);

	string var_str((char*)(*dataPtr), str_size);
	(*dataPtr) += str_size * sizeof(char);

	return var_str;
}

inline uint64_t GetLastSlash( const string& s )
{
	auto slash = s.rfind('\\');
	if(slash == string::npos)
		slash = 0;
	auto Rslash = s.rfind('/');
	if(Rslash != string::npos)
		slash = max<string::size_type>(slash, Rslash);
	return slash;
}

inline uint64_t GetFirstSlash( const string& s )
{
	return min<string::size_type>(s.find('\\'), s.find('/'));
}

inline bool DivideString( const string& s, string& first, string& second )
{
	auto slash = GetFirstSlash(s);
	first = s.substr(0, slash);
	if(slash != string::npos)
	{
		second = s.substr(slash);
		return true;
	}
	return false;
}

inline string GetFilename( const string& s )
{
	auto slash = GetLastSlash(s);
	string fileName;

	if(slash != string::npos)
	{
		string targetDir = s.substr(0, slash);
		fileName = s.substr(slash);			
	}
	else
	{
		fileName = s;
	}
	return fileName;
}

inline string RemoveExtension( const string& s )
{
	return s.substr(0, s.rfind('.'));
}

inline string GetExtension( const string& s )
{
	return s.substr(s.rfind('.'));
}

inline wstring StringToWstring( const string& s )
{
	int len;
    int slength = (int)s.length();
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

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

inline std::string UintToString(uint32_t i)
{
	char str[SRT_SIZE];
	sprintf_s(str, "%u", i);
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

inline Vector3 CharToVector3(char *str)
{
	Vector3 res = Vector3(0.0f,0.0f,0.0f);
	
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

inline void Vector3ToChar(char* str, Vector3 f)
{
	if(!str)return;
	sprintf_s(str, SRT_SIZE, "%f %f %f", f.x, f.y, f.z); 
}

inline string Vector3ToString(Vector3 f)
{
	char str[SRT_SIZE];
	sprintf_s(str, SRT_SIZE, "%f %f %f", f.x, f.y, f.z); 
	return string(str);  
}

inline Vector4 CharToVector4(char *str)
{
	Vector4 res = Vector4(0.0f,0.0f,0.0f,0.0f);
	
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

inline void Vector4ToChar(char* str, Vector4 f)
{
	if(!str)return;
	sprintf_s(str, SRT_SIZE, "%f %f %f %f", f.x, f.y, f.z, f.w); 
}

inline string Vector4ToString(Vector4 f)
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

inline uint32_t CharToUint(char *str)
{
	return (uint32_t)stol(str);
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

inline btTransform ToBtTransform(const XMMATRIX& mat)
{
	XMVECTOR scale, rot, pos;
	XMMatrixDecompose(&scale, &rot, &pos, mat);
	return btTransform(Quaternion(rot), Vector3(pos));
}

inline XMMATRIX ToXMMATRIX(const btTransform& transf)
{
	Vector3 pos = transf.getOrigin();
	Quaternion rot = transf.getRotation();
	return XMMatrixMultiply(XMMatrixRotationQuaternion(rot), XMMatrixTranslationFromVector(pos));
}