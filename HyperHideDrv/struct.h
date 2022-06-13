#pragma once

//����
#define MAX_PATH          260
/*==============================================================================*/
/*                                �Զ���ṹ                                    */
/*==============================================================================*/

 typedef struct _LIST_LINK  //�Զ��嵥������ڵ�
 {
	 PVOID DataPtr;
	 PVOID Data;//��������
	 struct _LIST_LINK* next;
 } LIST_LINK, * PLIST_LINK;
 typedef struct _LIST_HEAD  //�Զ��嵥������ͷ
 {
	 struct _LIST_LINK ListHead;
	 ULONG_PTR m_Size;

 } LIST_HEAD, * PLIST_HEAD;
 typedef struct _LIST_ARRAY//�Զ��嶯̬����
 {
	 PVOID DataPtr;//����ָ��
	 ULONG_PTR m_CurrentNumber;//��ǰԪ�ظ���
	 ULONG_PTR m_Size;//������������

 }LIST_ARRAY, * PLIST_ARRAY;


 typedef struct _TZM {
	 UCHAR	Tzm;
	 int		Offset;
 }TZM, * PTZM;


 typedef struct _ADDRESS_NAME  //R3������Žṹ
 {
	 IN	char  Name[MAX_PATH];
	 PVOID Address;

 }ADDRESS_NAME, * PADDRESS_NAME;