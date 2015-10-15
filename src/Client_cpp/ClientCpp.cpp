#include "ClientCpp.h"

#include <iostream>
#include <string.h>
#include <string>
#include <stdint.h>
#include <vector>

#include "../Caravel/Digest.h"
#include "../Caravel/PRF.h"
#include "../Caravel/AES.h"

using namespace std;
using namespace caravel;


ClientCpp::ClientCpp()
{
}


ClientCpp::~ClientCpp()
{
}

void ClientCpp::InitKey(string stKey)
{

	//init security key

	string stMkey;

	stMkey = stKey + "PK1";
	Digest::Sha256(stMkey, m_szPk1, SHA256_DIGEST_LENGTH);

	stMkey = stKey + "PK2";
	Digest::Sha256(stMkey, m_szPk2, SHA256_DIGEST_LENGTH);

	stMkey = stKey + "PK3";
	Digest::Sha256(stMkey, m_szPk3, SHA256_DIGEST_LENGTH);

	stMkey = stKey + "PK4";
	Digest::Sha256(stMkey, m_szPk4, SHA256_DIGEST_LENGTH);

	stMkey = stKey + "PK5";
	Digest::Sha256(stMkey, m_szPk5, SHA256_DIGEST_LENGTH);


}

void ClientCpp::Get(string &_retVal, string stTable, string stKey, string stCol)
{
	//Generate Trapdoor

	string stTmp = stTable + stKey + stCol;
	char szTd[SHA256_DIGEST_LENGTH];
	PRF::Sha256(m_szPk1, SHA256_DIGEST_LENGTH, (char*)stTmp.c_str(), stTmp.length(), szTd, SHA256_DIGEST_LENGTH);

	//Set Trapdoor to String
	string strTrapdoor;
	strTrapdoor.assign(szTd, SHA256_DIGEST_LENGTH);

	//Check the Router to choose a server to send request.
	uint32_t uiKey = *(uint32_t*)szTd;

	ThriftAdapt<TProxyServiceClient> *pThriftAdapt = *(m_SimConHash.Query(uiKey));
	TProxyServiceClient* pClient = pThriftAdapt->GetClient();

	//Send the request to remote server
	string strRet;
	pClient->ProxyGet(strRet, strTrapdoor);

	//we assume strRet.length == 0 means the key do not exist.
	if (strRet.length != 0)
	{
		//Decrypt the data
		m_Decrypt(strRet, _retVal);
	}


}

void ClientCpp::Put(string stTable, string stKey, string stCol, char *pVal, uint32_t uiLen, bool bIndex)
{
	//Generate the SendKey
	string stTmp = stTable + stKey + stCol;
	char szTd[SHA256_DIGEST_LENGTH];
	PRF::Sha256(m_szPk1, SHA256_DIGEST_LENGTH, (char*)stTmp.c_str(), stTmp.length(), szTd, SHA256_DIGEST_LENGTH);
	string strTrapdoor;
	strTrapdoor.assign(szTd, SHA256_DIGEST_LENGTH);

	//AES the data
	string strSendVal;
	m_Encrypt(strSendVal, pVal, uiLen);

	//Check the Router to choose a server to send request.
	uint32_t uiKey = *(uint32_t*)szTd;
	ThriftAdapt<TProxyServiceClient> *pThriftAdapt = *(m_SimConHash.Query(uiKey));
	TProxyServiceClient* pClient = pThriftAdapt->GetClient();

	if(bIndex)
	{
		
		//Generate the Index Trapdoor
		stTmp = stTable + stCol;
		char szIndexTd[SHA256_DIGEST_LENGTH];




	}
	


}

void ClientCpp::GetCol(vector<string> &_retVal, string stTable, string stCol, uint32_t uiNum)
{

}

void ClientCpp::Open()
{
	ThriftAdapt<TProxyServiceClient> **parThriftAdapt = m_SimConHash.GetArray();
	uint32_t uiNodeNum = m_SimConHash.GetNodeNum();
	for (uint32_t uiCur = 0; uiCur < uiNodeNum; uiCur++)
	{
		parThriftAdapt[uiCur]->Open();
	}
}


void ClientCpp::Close()
{
	ThriftAdapt<TProxyServiceClient> **parThriftAdapt = m_SimConHash.GetArray();
	uint32_t uiNodeNum = m_SimConHash.GetNodeNum();
	for (uint32_t uiCur = 0; uiCur < uiNodeNum; uiCur++)
	{
		parThriftAdapt[uiCur]->Close();
	}
}

void ClientCpp::InitExample()
{
	//DEMO_SERVER_NUM
	const uint32_t uiMax = (uint32_t)-1;
	uint32_t uiInterval = uiMax / DEMO_SERVER_NUM;

	for (uint32_t uiCur = 0; uiCur < DEMO_SERVER_NUM; uiCur++)
	{
		ThriftAdapt<TProxyServiceClient> *pThriftAdapt = new ThriftAdapt<TProxyServiceClient>();
		pThriftAdapt->Init(kDemoServerIP[uiCur], kDemoServerPort[uiCur]);
		//Add a node to SimConHash
		m_SimConHash.InsertNode(uiInterval * (uiCur + 1), pThriftAdapt);

	}

}


void ClientCpp::m_Decrypt(string &strCiphertext, string &strPlaintext)
{
	//Decrypt the AES
	char *pDec = new char[strCiphertext.length];

	memset(pDec, 0, strCiphertext.length);

	uint32_t uiSize = AES::CbcDecrypt256(strCiphertext.c_str(), strCiphertext.length(), pDec, m_szPk5);

	strPlaintext.assign(pDec, uiSize);

	delete[] pDec;

}



void ClientCpp::m_Encrypt(string &strCiphertext, char *pPlaintext, uint32_t uiPlaintextLen)
{

	char *pEnc = new char[AES::CbcMaxsize(uiPlaintextLen)];
	
	memset(pEnc, 0, AES::CbcMaxsize(uiPlaintextLen));
	
	uint32_t uiEncLen = AES::CbcEncrypt256(pPlaintext, uiPlaintextLen, pEnc, m_szPk5);

	strCiphertext.assign(pEnc, uiEncLen);

	delete[] pEnc;
}
