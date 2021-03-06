﻿/*===============================================*/
/* interpreter2.cpp                              */
/*===============================================*/

#include "interpreter2.h"

struct ForGosubStack ForGosub_s[FORGOSUB_S_MAX];
unsigned int ForGosub_sl=0;

#ifdef __cplusplus
extern "C"{
#endif

/*===関数定義===*/

int Str2VarID(st arg){
	int cnt=0;
	for(cnt=0;cnt<(VAR_MAX-Psys_FREEVAR/4096);cnt++){
		if(mystrcmp(Variable[cnt].name,arg)==0){
			return cnt;
		}
	}
	return -1;
}

uint16_t* GetVarID(uint16_t* p,int* tmpint,int* errtmp){
	int tmpints[10],tmpstr_p=0;
	char tmpstr[STR_LEN_MAX];
	char p_char=Code2Char(*p);
	st str={0,""};
	memset(tmpstr,0x00,sizeof(tmpstr));
	*errtmp=ERR_NO_ERROR;
	while(isalpha(p_char)||(p_char=='_')||(isdigit(p_char))){
		if((tmpstr_p==0)&&(isdigit(p_char))){
			*tmpint=-1;
			*errtmp=ERR_SYNTAX_ERROR;
			return p;
		}
		if(tmpstr_p>=8){
			*errtmp=ERR_STRING_TOO_LONG;
			return p;
		}
		tmpstr[tmpstr_p]=toupper(p_char);
		tmpstr_p++;
		p++;
		p_char=Code2Char(*p);
	}
	p_char=Code2Char(*p);
	if(p_char=='$'){
		tmpstr[tmpstr_p]=p_char;
		tmpstr_p++;
		p++;
	}
	p_char=Code2Char(*p);
	if(p_char=='('||p_char=='['){
		tmpstr[tmpstr_p]='(';
		tmpstr_p++;
		mystrcpy2(&str,tmpstr);
		tmpints[0]=Str2VarID(str);
		*tmpint=tmpints[0];
		if(tmpints[0]==-1){
			tmpints[0]=NewVar(str);
			RegistDim(tmpints[0],10,0);
		}
		*errtmp=PushCalcStack(TYPE_DIM_PTR,tmpints[0],MYSTR_NULL,0);
		if(*errtmp!=ERR_NO_ERROR)return p;
		p=ReadFormula(p,errtmp);
		if(*errtmp!=ERR_NO_ERROR)return p;
		*errtmp=ProcessRemainingOperator();
		if(*errtmp!=ERR_NO_ERROR)return p;
	}else{
		mystrcpy2(&str,tmpstr);
		p=JumpSpace(p);
		*tmpint=Str2VarID(str);
		if(*tmpint!=-1){
			//既存
			if(tmpstr[tmpstr_p-1]=='$'){
				*errtmp=PushCalcStack(TYPE_STR_VAR,*tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				*errtmp=PushCalcStack(TYPE_INT_VAR,*tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}
		}else{
			//新規登録
			*tmpint=NewVar(str);
			if(Variable[*tmpint].isStr){
				*errtmp=PushCalcStack(TYPE_STR_VAR,*tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				*errtmp=PushCalcStack(TYPE_INT_VAR,*tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}
		}
	}
	return p;
}

void IncrementSrcPos(void){
	if(*srcpos==0x000D){
		cur_line++;
		GOTOLINE(cur_line);
		return;
	}
	if(*srcpos==0x0000){
		return;
	}
	srcpos++;
	return;
}

int ProcessRemainingOperator(void){
	int argcount=0,errtmp;	
	uint16_t op=0;
	while(op_sl>0)  {
		PopOpStack(&op,&argcount);
		if((op==Char2Code('('))||(op==Char2Code(')'))) {
			//括弧が対応していない
			return ERR_SYNTAX_ERROR;
		}
		errtmp=EvalFormula(op,argcount);
		if(errtmp!=ERR_NO_ERROR)return errtmp;
	}
	return ERR_NO_ERROR;
}

uint16_t* JumpSpace(uint16_t* p){
	for(;(*p==Char2Code(' '));p++){}
	return p;
}

uint16_t* ForJump(uint16_t* p,int* errtmp){
	int nest=0;
	unsigned int i=0,f=0,flag=0;
	*errtmp=ERR_NO_ERROR;
	for(i=cur_line+1;i<(srclinecount-1);i++){
		if(flag!=0)p=translated_source+srcline_begin_token_pos[i];
		flag=1;
		while(*p!=0x000D && *p!=TOKEN_REM && *p!=TOKEN_REM2){
			if(*p==TOKEN_FOR)nest++;
			if(*p==TOKEN_NEXT){
				if(nest==0){
					cur_line=i;
					f=1;
					break;
				}
				nest--;
			}
			p++;
		}
		if(f==1)break;
	}
	if(f==0){
		*errtmp=ERR_FOR_WITHOUT_NEXT;
		return p+1;
	}
	return p+1;
}

uint16_t* ReadFormula(uint16_t* p,int *errtmp){
	char tmpstr[STR_LEN_MAX],tmpstr2[512];
	char tmpstr_p=0;
	st str={0,""};
	uint16_t *pbegin=p;
	int cnt=0,tmp=0,beforetokentype=0,nest_depth=0;
	char argcount[100],brackettype[100];
	char p_char=0;
	int32_t tmpint=0;
	int64_t tmpint2=0;
	double tmpw=0;
	*errtmp=ERR_NO_ERROR;
	memset(tmpstr, 0x00,sizeof(tmpstr));
	memset(tmpstr2,0x00,sizeof(tmpstr2));
	memset(argcount,0x00,sizeof(argcount));
	memset(brackettype,0x00,sizeof(brackettype));
	mystrclear(&str);
	while(*p!=0){
		p_char=Code2Char(*p);
		if(isalpha(p_char)||(p_char=='_')){
			mystrclear(&str);
			cnt=0;
			while((isalpha(p_char)||(p_char=='_')||isdigit(p_char))&&(*p!=0)){
				if(cnt>=8){
					*errtmp=ERR_STRING_TOO_LONG;
					return p;
				}
				str.s[cnt]=toupper(p_char);
				str.len++;
				p++;
				p_char=Code2Char(*p);
				cnt++;
			}
			p_char=Code2Char(*p);
			if(p_char=='$'){ 
				str.s[cnt]=p_char;
				str.len++;
				p++;
				p_char=Code2Char(*p);
				cnt++;
			}
			p_char=Code2Char(*p);
			if(p_char=='(' || p_char=='['){
				str.s[cnt]='(';
				str.len++;
				tmpint=Str2VarID(str);
				if((beforetokentype==TYPE_INT_LIT)||(beforetokentype==TYPE_STR_LIT)){
					//Next statement
					//行き過ぎたのを戻す
					p-=cnt;
					return p;
				}
				if(tmpint==-1){
					tmpint=NewVar(str);
					RegistDim(tmpint,10,0);
				}
				*errtmp=PushCalcStack(TYPE_DIM,tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
				beforetokentype=TYPE_FUNC;//本来はTYPE_DIMだが便宜的に。
			}else{
				tmp=Str2VarID(str);
				if(tmp!=-1){
					if((beforetokentype==TYPE_INT_LIT)||(beforetokentype==TYPE_STR_LIT)){
						//Next statement
						//行き過ぎたのを戻す
						p-=cnt;
						return p;
					}
					if(Variable[tmp].isStr){
						*errtmp=PushCalcStack(TYPE_STR_LIT,0,Variable[tmp].string,0);
						if(*errtmp!=ERR_NO_ERROR)return p;
						beforetokentype=TYPE_STR_LIT;
					}else{
						*errtmp=PushCalcStack(TYPE_INT_LIT,Variable[tmp].value,MYSTR_NULL,0);
						if(*errtmp!=ERR_NO_ERROR)return p;
						beforetokentype=TYPE_INT_LIT;
					}
				}else{
					if((beforetokentype==TYPE_INT_LIT)||(beforetokentype==TYPE_STR_LIT)){
						//Next statement
						//行き過ぎたのを戻す
						p-=cnt;
						return p;
					}
					tmpint=NewVar(str);
					if(Variable[tmpint].isStr){
						*errtmp=PushCalcStack(TYPE_STR_LIT,0,MYSTR_NULL,0);
						if(*errtmp!=ERR_NO_ERROR)return p;
						beforetokentype=TYPE_STR_LIT;
					}else{
						*errtmp=PushCalcStack(TYPE_INT_LIT,0,MYSTR_NULL,0);
						if(*errtmp!=ERR_NO_ERROR)return p;
						beforetokentype=TYPE_INT_LIT;
					}
				}
				p=JumpSpace(p);
			}
			p_char=Code2Char(*p);
		}else if(isdigit(p_char)){
			if(beforetokentype==TYPE_INT_LIT){
				return p;
			}
			tmpint=0;
			cnt=0;
			while(isdigit(p_char)){
				if(cnt>=7){
					*errtmp=ERR_OVERFLOW;
					return p;
				}
				tmpint=(tmpint*10)+(p_char-'0');
				p++;p_char=Code2Char(*p);cnt++;
			}
			if(tmpint>=524288){
				*errtmp=ERR_OVERFLOW;
				return p;
			}
			tmpint*=4096;
			if(p_char=='.'){
				p++;
				p_char=Code2Char(*p);
				tmpw=0;
				for(cnt=0;isdigit(Code2Char(*(p+cnt)))&&(cnt<=6);cnt++);
				cnt--;
				tmp=cnt;
				while(cnt>=0){
					tmpw=((tmpw+(double)(Code2Char(*(p+cnt))-'0'))/10.0);
					cnt--;
				}
				p+=tmp;
				p_char=Code2Char(*p);
				//切り捨てられるものに最小分解能の1/4096の1/2を足すことで
				//四捨五入する
				tmpw+=(1.0/8192.0);
				tmpw*=4096.0;
				tmpint+=(int)tmpw;
				for(;isdigit(Code2Char(*p));p++);
				p_char=Code2Char(*p);
			}
			*errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			p=JumpSpace(p);
			p_char=Code2Char(*p);
			beforetokentype=TYPE_INT_LIT;
		}else if(p_char=='&'){
			p++;
			p_char=Code2Char(*p);
			if(toupper(p_char)=='H'){
				p++;
				p_char=Code2Char(*p);
				tmpint=0;
				cnt=0;
				while(isxdigit(p_char)){
					if(cnt>=5){
						*errtmp=ERR_SYNTAX_ERROR;
						return p;
					}
					tmpint=(tmpint<<4)|((p_char<='9')?(p_char-'0'):(toupper(p_char)-'A'+10));
					p++;p_char=Code2Char(*p);cnt++;
				}
				tmpint*=4096;
			}else if(toupper(p_char)=='B'){
				p++;
				p_char=Code2Char(*p);
				tmpint=0;
				cnt=0;
				while(isBin(p_char)){
					if(cnt>=20){
						*errtmp=ERR_SYNTAX_ERROR;
						return p;
					}
					tmpint=(tmpint<<1)|(p_char-'0');
					p++;p_char=Code2Char(*p);cnt++;
				}
				tmpint*=4096;
			}else{
				*errtmp=ERR_SYNTAX_ERROR;
				return p;
			}
			*errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			beforetokentype=TYPE_INT_LIT;
			p=JumpSpace(p);
			p_char=Code2Char(*p);
		}else if(p_char=='"'){
			if((beforetokentype==TYPE_INT_LIT)||(beforetokentype==TYPE_STR_LIT)||(beforetokentype==TYPE_SPECIAL2)){
				*errtmp=ERR_NO_ERROR;
				return p;
			}
			p++;
			p_char=Code2Char(*p);
			memset(tmpstr, 0x00,sizeof(tmpstr));
			tmpstr_p=0;
			while((p_char!='"')&&(p_char!=0)){

				//ソースは最大でも一行100文字のため、
				//ここでSTR_LEN_MAX文字を超えることはない
				tmpstr[tmpstr_p]=Code2Char(*p);
				tmpstr_p++;
				p++;	
				p_char=Code2Char(*p);
			}
			if(p_char=='"'){p++;p_char=Code2Char(*p);}
			*errtmp=PushCalcStack(TYPE_STR_LIT,0,str2mystr2(tmpstr),0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			beforetokentype=TYPE_STR_LIT;
		}else if(p_char=='<'){
			if(*(p+1)==Char2Code('=')){
				p++;
				p_char=Code2Char(*p);
				*errtmp=PushCalcStack(TYPE_FUNC,OP_SHONARI_EQUAL,MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				*errtmp=PushCalcStack(TYPE_FUNC,Char2Code('<'),MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}
			beforetokentype=TYPE_FUNC;
			p++;
		}else if(p_char=='>'){
			if(*(p+1)==Char2Code('=')){
				p++;
				p_char=Code2Char(*p);
				*errtmp=PushCalcStack(TYPE_FUNC,OP_DAINARI_EQUAL,MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				*errtmp=PushCalcStack(TYPE_FUNC,Char2Code('>'),MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}
			p++;
			p_char=Code2Char(*p);
			beforetokentype=TYPE_FUNC;
		}else if(p_char=='='){
			if(*(p+1)==Char2Code('=')){
				p++;
				p_char=Code2Char(*p);
				*errtmp=PushCalcStack(TYPE_FUNC,OP_EQUAL,MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				*errtmp=ERR_NO_ERROR;
				return p;
			}
			p++;
			p_char=Code2Char(*p);
			beforetokentype=TYPE_FUNC;
		}else if(p_char=='!'){
			if(*(p+1)==Char2Code('=')){
				p++;
				p_char=Code2Char(*p);
				*errtmp=PushCalcStack(TYPE_FUNC,OP_NOTEQUAL,MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
				beforetokentype=TYPE_FUNC;
			}else{
				*errtmp=ERR_SYNTAX_ERROR;
				return p;
			}
			p++;
		}else if(p_char==','){
			if(nest_depth<0){
				//?
				*errtmp=ERR_NO_ERROR;
				return p;
			}else if(nest_depth==0){
				*errtmp=ERR_NO_ERROR;
				return p;
			}else{
				argcount[nest_depth-1]++;
				p=JumpSpace(p+1);
				p_char=Code2Char(*p);
				*errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code(','),MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
				beforetokentype=TYPE_SPECIAL;
			}
		}else if((p_char==':')||(p_char==';')||(p_char=='\'')||(p_char=='?')){
			if(pbegin==p){
				//式なし
				*errtmp=PushCalcStack(TYPE_VOID,0,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
				return p;
			}else{
				*errtmp=ERR_NO_ERROR;
				return p;
			}
		}else if(p_char=='('){
			brackettype[nest_depth]=0;
			*errtmp=PushCalcStack(TYPE_FUNC,Char2Code('('),MYSTR_NULL,0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			p=JumpSpace(p+1);
			p_char=Code2Char(*p);
			nest_depth++;
			beforetokentype=TYPE_SPECIAL;
		}else if(p_char==')'){
			nest_depth--;
			if(nest_depth<0){
				//これが最初ならばエラーにしたい
				if(beforetokentype==0)*errtmp=ERR_SYNTAX_ERROR;
				*errtmp=ERR_NO_ERROR;
				return p;
			}
			if(brackettype[nest_depth]!=0){
				*errtmp=ERR_SYNTAX_ERROR;
				return p;
			}
			if(beforetokentype==TYPE_SPECIAL){
				*errtmp=PushCalcStack(TYPE_VOID,0,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
			}else{
				argcount[nest_depth]++;
			}
			*errtmp=PushCalcStack(TYPE_FUNC,Char2Code(')'),MYSTR_NULL,argcount[nest_depth]);
			if(*errtmp!=ERR_NO_ERROR)return p;
			argcount[nest_depth]=0;
			p=JumpSpace(p+1);
			p_char=Code2Char(*p);
			beforetokentype=TYPE_SPECIAL2;
		}else if(p_char=='['){
			brackettype[nest_depth]=1;
			*errtmp=PushCalcStack(TYPE_FUNC,Char2Code('('),MYSTR_NULL,0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			p=JumpSpace(p+1);
			p_char=Code2Char(*p);
			nest_depth++;
			beforetokentype=TYPE_SPECIAL;
		}else if(p_char==']'){
			nest_depth--;
			if(brackettype[nest_depth]!=1){
				*errtmp=ERR_SYNTAX_ERROR;
				return p;
			}
			argcount[nest_depth]++;
			*errtmp=PushCalcStack(TYPE_FUNC,Char2Code(')'),MYSTR_NULL,argcount[nest_depth]);
			if(*errtmp!=ERR_NO_ERROR)return p;
			p=JumpSpace(p+1);
			p_char=Code2Char(*p);
			beforetokentype=TYPE_SPECIAL2;
		}else if(p_char=='-'){
			if((beforetokentype==0)||(beforetokentype==TYPE_FUNC)||(beforetokentype==TYPE_SPECIAL)){
				*errtmp=PushCalcStack(TYPE_FUNC,OP_MINUSSIGN,MYSTR_NULL,1);
				if(*errtmp!=ERR_NO_ERROR)return p;
				p++;p_char=Code2Char(*p);
				beforetokentype=TYPE_FUNC;
			}else{
				*errtmp=PushCalcStack(TYPE_FUNC,Char2Code('-'),MYSTR_NULL,2);
				if(*errtmp!=ERR_NO_ERROR)return p;
				p++;p_char=Code2Char(*p);
				beforetokentype=TYPE_FUNC;
			}
		}else if(isOperator(*p)){
			*errtmp=PushCalcStack(TYPE_FUNC,*p,MYSTR_NULL,GetOperatorArgCount(*p));
			if(*errtmp!=ERR_NO_ERROR)return p;
			p++;p_char=Code2Char(*p);
			beforetokentype=TYPE_FUNC;
		}else if(isFunction(*p)){
			*errtmp=PushCalcStack(TYPE_FUNC,*p,MYSTR_NULL,0);
			if(*errtmp!=ERR_NO_ERROR)return p;
			p=JumpSpace(p+1);
			p_char=Code2Char(*p);
			beforetokentype=TYPE_FUNC;
		}else if(isSystemVariable(*p)){
			tmp=GetSystemVariableType(*p);
			switch(tmp){
			case 1:case 2:
				tmpint=*(GetSystemVariableIntPtr(*p));
				if((*p==TOKEN_CSRX)||(*p==TOKEN_CSRY))tmpint*=4096;
				*errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
				beforetokentype=TYPE_INT_LIT;
				p++;p_char=Code2Char(*p);
				break;
			case 3:case 4:
				str=*(GetSystemVariableStrPtr(*p));
				*errtmp=PushCalcStack(TYPE_STR_LIT,0,str,0);
				if(*errtmp!=ERR_NO_ERROR)return p;
				beforetokentype=TYPE_STR_LIT;
				p++;p_char=Code2Char(*p);
				break;
			default:
				p++;
				break;
			}
		}else if(*p==0x0020){
			p++;
		}else if(*p==0x0000 || *p==0x000D){
			*errtmp=ERR_NO_ERROR;
			return p;
		}else{
			*errtmp=ERR_NO_ERROR;
			return p;
		}
	}
	return p;
}

void TranslateRaw2Code(unsigned char* input,uint16_t* output,int *outlen){
	unsigned char *inpos=input;
	uint16_t *outpos=output;
	unsigned char *srcend=input+strlen((char*)input);
	unsigned char c=0;
	char tmpstr[STR_LEN_MAX];
	unsigned char tmpstr_p=0;
	int cnt=0,i=0,j=0;
	uint16_t codetmp;
	int32_t tmpint=0;
	memset(tmpstr,0x00,sizeof(tmpstr));
	memset(output,0x0000,sizeof(output));
	memset(srcline_begin_token_pos, 0x0000,sizeof(srcline_begin_token_pos));
	memset(srcline_token_count, 0x0000,sizeof(srcline_token_count));
	srclinecount=0;
	while(*inpos!=0x00){
		c=*inpos;
		if(isalpha(c)){
			i=0;
			memset(tmpstr, 0x00,sizeof(tmpstr));
			while((i<8)&&(isalpha(c)||isdigit(c)||c=='_')){
				if(islower(inpos[i])){
					tmpstr[i]=toupper(inpos[i]);
				}else{
					tmpstr[i]=inpos[i];
				}
				i++;
				c=*(inpos+i);
			}
			if(c=='$'){
				tmpstr[i]=c;
				i++;
			}
			//IDに変換
			if(Str2TokenCode(str2mystr2(tmpstr),&codetmp)){
				*outpos=codetmp;
				outpos++;
				srcline_token_count[srclinecount]++;
				inpos+=i;
			}else{
				for(j=0;j<i;j++){
					if(islower(inpos[j])){
						*outpos=Char2Code(toupper(tmpstr[j]));
					}else{
						*outpos=Char2Code(tmpstr[j]);
					}
					outpos++;
					srcline_token_count[srclinecount]++;
					inpos++;
				}
			}
		}else if(c=='"'){
			*outpos=Char2Code(c);
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
			c=*inpos;
			while((c!=0x00)&&(c!=0x0D)&&(c!='"')){
				*outpos=Char2Code(c);
				outpos++;
				srcline_token_count[srclinecount]++;
				inpos++;
				c=*inpos;
			}
			*outpos=Char2Code('"');
			outpos++;
			srcline_token_count[srclinecount]++;
			if(c!=0x0D && c!=0x00)inpos++;
		}else if(c=='?'){
			*outpos=TOKEN_PRINT2;
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
		}else if(c=='&'){
			*outpos=Char2Code(c);
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
			*outpos=Char2Code(*inpos);
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
		}else if(c=='@'){
			*outpos=TOKEN_LABEL2;
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
		}else if(c==0x0D){
			inpos++;
			c=*inpos;
			if(c==0x0A)inpos++;
			*outpos=0x000D;
			outpos++;
			srcline_token_count[srclinecount]++;
			srclinecount++;
			srcline_begin_token_pos[srclinecount]=srcline_begin_token_pos[srclinecount-1]+srcline_token_count[srclinecount-1];
		}else if(c==0x0A){
			inpos++;
		}else{
			*outpos=Char2Code(*inpos);
			outpos++;
			srcline_token_count[srclinecount]++;
			inpos++;
		}

	};
	*outpos=0x0000;
	outpos++;
	srclinecount++;
	*outlen=outpos-output;
	if(log_en){
		for(i=0;i<*outlen;i++)printf("%X,",output[i]);
		puts("");
	}
	return;
}

void TranslateCode2Raw(uint16_t* input,unsigned char* output){
	uint16_t *inpos=input;
	unsigned char *outpos=output;
	st str={0,""};
	char tmp;
	char tmpstr[10];
	memset(output,0x00,sizeof(output));
	while(*inpos!=0x0000){
		tmp=Code2Char(*inpos);
		if(tmp==0x00){
			str=TokenCode2Str(*inpos);
			strcpy(tmpstr,(char*)str.s);
			if(strcmp(tmpstr,"")==0){
				if(*inpos==0x000D){
					*outpos=0x0D;
					outpos++;
					*outpos=0x0A;
					outpos++;
				}
			}
			strcat((char*)outpos,tmpstr);
		}else{
			*outpos=tmp;
			outpos++;
		}
		input++;	
	}
	return;
}

void RunInteractive(st input){
	st str;
	uint16_t codedata[33];
	int codelen=0,errtmp=ERR_NO_ERROR,runflag=0;
	char tmpstr[STR_LEN_MAX];
	unsigned char tmpstr2[STR_LEN_MAX];
	error_occured_token=0;
	breakflag=0;
	read_initialized=false;
	memset(tmpstr,0x00,sizeof(char)*256);
	memset(tmpstr2,0x00,sizeof(char)*256);
	mystr2str2(input,tmpstr);
	memcpy(tmpstr2,tmpstr,256);
	TranslateRaw2Code(tmpstr2,codedata,&codelen);
	runmode=RMD_LINE;
	if(Psys_CSRX!=0)Print2Console(MYSTR_NULL,0);
	errtmp=Interpret(codedata,codelen,true,&runflag);
	ClearKeyBuffer();
	runmode=RMD_STOP;
	if(runflag==1){
		runmode=RMD_PRG;
		errtmp=RunProgram();
		ClearKeyBuffer();
		if(breakflag==1){
			memset(tmpstr,0x00,sizeof(char)*256);
			sprintf(tmpstr,"BREAK in %d",cur_line+1);
			if(Psys_CSRX!=0)Print2Console(MYSTR_NULL,0);
			printf("%s\n",tmpstr);
			Print2Console(str2mystr2(tmpstr),0);
			ConsoleClearLine();
			runmode=RMD_STOP;
			return;
		}
		runmode=RMD_STOP;
		Psys_ERR=errtmp;
		if((errtmp==ERR_NO_ERROR)&&(op_sl==0)&&(calc_sl==0)){
			if(Psys_CSRX!=0)Print2Console(MYSTR_NULL,0);
		}else{
			Psys_ERL=cur_line+1;
			printf("op_sl:%d calc_sl:%d",op_sl,calc_sl);
			memset(tmpstr,0x00,sizeof(tmpstr));
			if(Psys_CSRX!=0)Print2Console(MYSTR_NULL,0);
			if(error_occured_token!=0){
				sprintf(tmpstr,"%s (%d,%s)",GetErrorMessage(errtmp).s,Psys_ERL,TokenCode2Str(error_occured_token).s);
				printf("%s\n",tmpstr);
				printf("*srcpos=%04X=%c\n",*srcpos,Code2Char(*srcpos));
				Print2Console(str2mystr2(tmpstr),0);
			}else{
				printf("*srcpos=%04X=%c\n",*srcpos,Code2Char(*srcpos));
				printf("%s\n",GetErrorMessage(errtmp).s);
				Print2Console(GetErrorMessage(errtmp),0);
			}
			if(Psys_SYSBEEP)PlaySoundMem(SHandleBEEP[2],DX_PLAYTYPE_BACKBIT);
		}
		puts("OK");
		Print2Console(str2mystr2("OK"),0);
		ConsoleClearLine();
		return;
	}else{
		Psys_ERR=errtmp;
		if(errtmp==ERR_NO_ERROR){
			if((op_sl!=0)||(calc_sl!=0)){
				printf("op_sl:%d calc_sl:%d",op_sl,calc_sl);
				str=GetErrorMessage(ERR_UNDEFINED);
				printf("*srcpos=%04X=%c\n",*srcpos,Code2Char(*srcpos));
				printf("%s\n",tmpstr);
				Print2Console(str,0);
			}
		}else{
			memset(tmpstr,0x00,sizeof(tmpstr));
			if(error_occured_token!=0){
				str=TokenCode2Str(error_occured_token);
				if(str.len==0){
					str=GetErrorMessage(errtmp);
				}else{
					sprintf(tmpstr,"%s (%s)",GetErrorMessage(errtmp).s,TokenCode2Str(error_occured_token).s);
					str=str2mystr2(tmpstr);
				}
				printf("%s\n",str.s);
				printf("*srcpos=%04X=%c\n",*srcpos,Code2Char(*srcpos));
				Print2Console(str,0);
			}else{
				str=GetErrorMessage(errtmp);
				printf("%s\n",str.s);
				printf("*srcpos=%04X=%c\n",*srcpos,Code2Char(*srcpos));
				Print2Console(str,0);
			}
			if(Psys_SYSBEEP)PlaySoundMem(SHandleBEEP[2],DX_PLAYTYPE_BACKBIT);
		}
		puts("OK");
		Print2Console(str2mystr2("OK"),0);
		ConsoleClearLine();
		return;
	}
}

int RunProgram(void){
	int srclen,dummy=0,errtmp=ERR_NO_ERROR;
	error_occured_token=0;
	TranslateRaw2Code(source_ptr,translated_source,&srclen);
	errtmp=ResistLabel(translated_source);
	if(errtmp!=ERR_NO_ERROR)return errtmp;
	return Interpret(translated_source,srclen,false,&dummy);
}

int ResistLabel(uint16_t* input){
	uint16_t *p=input;
	unsigned int i=0,j=0;
	char tmpstr[16],c;
	labelcount=0;
	cur_line=0;
	memset(labellist_name,0x00,sizeof(labellist_name));
	for(i=0;i<srclinecount;i++){
		p=input+srcline_begin_token_pos[i];
		p=JumpSpace(p);
		if(*p==TOKEN_LABEL || *p==TOKEN_LABEL2){
			p++;
			memset(tmpstr,0x00,sizeof(tmpstr));
			for(j=0;j<8;j++){
				c=Code2Char(*p);
				if((c==' ')||(*p==0x000D)||(*p==0x0000))break;
				if((c!='_')&&(!isalpha(c))&&(!isdigit(c))){
					Psys_ERL=i+1;
					return ERR_SYNTAX_ERROR;
				}
				tmpstr[j]=toupper(c);
				p++;
			}
			strcpy(labellist_name[labelcount],tmpstr);
			labellist_line[labelcount]=i;
			labelcount++;
		}
	}
	return ERR_NO_ERROR;
}

uint16_t* GetLabelName(uint16_t* p,char* tmpstr,int* errtmp){
	int i=0;
	uint16_t c;
	memset(tmpstr,0x00,256);
	for(i=0;i<8;i++){
		c=Code2Char(*p);
		if((c==' ')||(*p==0x000D)||(*p==0x0000)||(c==':')||(c=='\''))break;
		if((c!='_')&&(!isalpha(c))&&(!isdigit(c))){
			*errtmp=ERR_SYNTAX_ERROR;
			return p;
		}
		tmpstr[i]=toupper(c);
		p++;
	}
	if(i==0){
		*errtmp=ERR_SYNTAX_ERROR;
		return p;
	}
	*errtmp=ERR_NO_ERROR;
	return p;
}

int ReadGotoLine(uint16_t* input,unsigned int line){
	int errtmp=ERR_NO_ERROR;
	read_srcpos=input+srcline_begin_token_pos[line];
	errtmp=ReadSeekNext();
	if(errtmp!=ERR_NO_ERROR)return errtmp;
	return ERR_NO_ERROR;
}

int ReadSeekNext(void){
	while(*read_srcpos!=TOKEN_DATA && *read_srcpos!=0x0000){
		//コメントは次行まで読み飛ばし
		if(*read_srcpos==TOKEN_REM ||*read_srcpos==TOKEN_REM2){
			while(*read_srcpos!=0x000D && *read_srcpos!=0x0000)read_srcpos++;
		}
		read_srcpos++;
	}
	if(*read_srcpos==0x0000){
		return ERR_OUT_OF_DATA;
	}
	read_srcpos=JumpSpace(read_srcpos+1);
	read_initialized=true;
	return ERR_NO_ERROR;
}

int Interpret(uint16_t* input,int srclen,bool interactive_flag,int* runflag){
	uint16_t *tmppos;
	const uint16_t *srcend=input+srclen;
	uint32_t tmpline=0;
	uint16_t t=0;
	char *c2,p_char;
	unsigned char c=0;
	char tmpstr[STR_LEN_MAX],tmpstr2[STR_LEN_MAX*2],tmpstr3[STR_LEN_MAX];
	unsigned char tmpstr_p=0;
	st str={0,""};
	double tmpw=0.0;
	int cnt=0,i=0,argcount=0;
	int32_t tmpint=0,tmpint3=0;
	int64_t tmpint2=0;
	int lastprintmode=0,errtmp=ERR_NO_ERROR;

	int state = ST_LINE_BEGIN;//状態変数

	memset(tmpstr, 0x00,sizeof(tmpstr));
	memset(tmpstr2, 0x00,sizeof(tmpstr2));
	memset(op_s,0x00,sizeof(op_s));
	memset(calc_s,0x00,sizeof(calc_s));
	srcpos=input;
	read_srcpos=input;
	calc_sl=0;
	op_sl=0;
	*runflag=0;

	while(srcpos<srcend){
		memset(&str,0x00,sizeof(st));
		t=*srcpos;
		error_occured_token=t;
		c=Code2Char(t);
		if(c==0x00){
			//Code
			switch(t){
			case TOKEN_PRINT:case TOKEN_PRINT2:
				srcpos++;
				while(srcpos<srcend){
					srcpos=JumpSpace(srcpos);
					tmpint=Code2Char(*srcpos);
					if((tmpint==',')||(tmpint==';'))srcpos++;
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(!PopCalcStack_void()){
						memset(tmpstr,0x00,sizeof(tmpstr));
						i=PopCalcStack_int(&tmpint);
						if(i){
							if(tmpint%4096==0){
								sprintf(tmpstr,"%d",tmpint/4096);
							}else{
								sprintf(tmpstr,"%.3f",(double)(tmpint/4096.0));
								for(i=strlen(tmpstr)-1;(i!=0)&&(tmpstr[i]=='0');i--){
									tmpstr[i]=0;
								}
								if(tmpstr[i]=='.')tmpstr[i]=0;
							}
							mystrcpy2(&str,tmpstr);
						}else{
							i=PopCalcStack_str(&str);
						}
						tmpint=0;
						switch(Code2Char(*srcpos)){
						case ';':
							lastprintmode=1;
							Print2Console(str,1);
							srcpos=JumpSpace(srcpos+1);
							if(*srcpos==0x000D){
								cur_line++;
								if(cur_line>=srclinecount)return ERR_NO_ERROR;
								GOTOLINE(cur_line);
								tmpint=1;
							}
							break;
						case ',':
							lastprintmode=2;
							Print2Console(str,2);
							srcpos=JumpSpace(srcpos+1);
							if(*srcpos==0x000D){
								cur_line++;
								if(cur_line>=srclinecount)return ERR_NO_ERROR;
								GOTOLINE(cur_line);
								tmpint=1;
							}
							break;
						case '\'':
							lastprintmode=0;
							Print2Console(str,0);
							cur_line++;
							if(cur_line>=srclinecount)return ERR_NO_ERROR;
							GOTOLINE(cur_line);
							tmpint=1;
							break;
						case ':':
							lastprintmode=0;
							Print2Console(str,0);
							srcpos++;
							tmpint=1;
							break;
						default:
							lastprintmode=0;
							tmpint=1;
							p_char=Code2Char(*srcpos);
							if((p_char=='_')||(p_char=='"')||isalpha(p_char)||isdigit(p_char)){
								Print2Console(str,1);
								tmpint=0;
								lastprintmode=1;
								break;
							}
							Print2Console(str,0);
							break;
						}
					}else{
						Print2Console(MYSTR_NULL,lastprintmode);
						lastprintmode=0;
						srcpos++;
						tmpint=1;
						break;
					}
					if(tmpint==1)break;
				}
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_DIM:
				do{
					srcpos=JumpSpace(srcpos+1);
					mystrclear(&str);
					p_char=Code2Char(*srcpos);
					while(isalpha(p_char)||(p_char=='_')||(isdigit(p_char))){
						if((str.len==0)&&(isdigit(p_char))){
							return ERR_SYNTAX_ERROR;
						}
						if(str.len>=8)return ERR_STRING_TOO_LONG;
						str.s[str.len]=toupper(p_char);
						str.len++;
						srcpos++;
						p_char=Code2Char(*srcpos);
					}
					p_char=Code2Char(*srcpos);
					if(p_char=='$'){
						str.s[str.len]='$';
						str.len++;
						srcpos++;
						tmpint=1;
					}else{
						tmpint=0;
					}
					if(Code2Char(*srcpos)=='('){
						str.s[str.len]='(';
						str.len++;
					}else if(Code2Char(*srcpos)=='['){
						str.s[str.len]='(';
						str.len++;
					}else{
						return ERR_SYNTAX_ERROR;
					}
					tmpint2=Str2VarID(str);
					if(tmpint2!=-1)return ERR_DUPLICATE_DEFINITION;
					errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_STR_LIT,0,str,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,TOKEN_DIM,MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					ProcessRemainingOperator();
				}while(Code2Char(*srcpos)==',');
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_RESTORE:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=TOKEN_LABEL && *srcpos!=TOKEN_LABEL2)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				srcpos=GetLabelName(srcpos,tmpstr,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				tmpint=0;
				for(i=0;i<labelcount;i++){
					if(strcmp(labellist_name[i],tmpstr)==0){
						ReadGotoLine(input,labellist_line[i]);
						tmpint=1;
						break;
					}
				}
				if(tmpint==0)return ERR_UNDEFINED_LABEL;
				break;
			case TOKEN_DATA:
				cur_line++;
				if(cur_line>=srclinecount)return ERR_NO_ERROR;
				GOTOLINE(cur_line);
				break;
			case TOKEN_READ:
				if(!read_initialized){
					errtmp=ReadSeekNext();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
				}
				do{
					srcpos=JumpSpace(srcpos+1);
					srcpos=GetVarID(srcpos,&tmpint,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					read_srcpos=JumpSpace(read_srcpos);
					p_char=Code2Char(*read_srcpos);
					tmpint2=1;//符号フラグとして使用
					if(inrange(p_char,'0','9')||(p_char=='-')){
						if(p_char=='-'){
							tmpint2=-1;
							read_srcpos++;
							p_char=Code2Char(*read_srcpos);
						}
						cnt=0;
						tmpint=0;
						tmpint3=0;
						while(isdigit(p_char)){
							if(cnt>=7){
								return ERR_OVERFLOW;
							}
							tmpint=(tmpint*10)+(p_char-'0');
							read_srcpos++;p_char=Code2Char(*read_srcpos);cnt++;
						}
						if(tmpint>=524288){
							return ERR_OVERFLOW;
						}
						tmpint*=4096*(int)tmpint2;
						if(p_char=='.'){
							read_srcpos++;
							p_char=Code2Char(*read_srcpos);
							tmpw=0;
							for(cnt=0;isdigit(Code2Char(*(read_srcpos+cnt)))&&(cnt<=6);cnt++);
							cnt--;
							tmpint3=cnt;
							while(cnt>=0){
								tmpw=((tmpw+(double)(Code2Char(*(read_srcpos+cnt))-'0'))/10.0);
								cnt--;
							}
							read_srcpos+=tmpint3;
							p_char=Code2Char(*read_srcpos);
							//切り捨てられるものに最小分解能の1/4096の1/2を足すことで
							//四捨五入する
							tmpw+=(1.0/8192.0);
							tmpw*=4096.0;
							tmpint+=(int)tmpw;
							for(;isdigit(Code2Char(*read_srcpos));read_srcpos++);
							p_char=Code2Char(*read_srcpos);
						}
						errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						read_srcpos=JumpSpace(read_srcpos);
						p_char=Code2Char(*read_srcpos);
					}else if(p_char=='&'){
						read_srcpos++;
						p_char=Code2Char(*read_srcpos);
						if(toupper(p_char)=='H'){
							read_srcpos++;
							p_char=Code2Char(*read_srcpos);
							tmpint=0;
							cnt=0;
							while(isxdigit(p_char)){
								if(cnt>=5){
									return ERR_SYNTAX_ERROR;
								}
								tmpint=(tmpint<<4)|((p_char<='9')?(p_char-'0'):(toupper(p_char)-'A'+10));
								read_srcpos++;p_char=Code2Char(*read_srcpos);cnt++;
							}
							tmpint*=4096;
						}else if(toupper(p_char)=='B'){
							read_srcpos++;
							p_char=Code2Char(*read_srcpos);
							tmpint=0;
							cnt=0;
							while(isBin(p_char)){
								if(cnt>=20){
									return ERR_SYNTAX_ERROR;
								}
								tmpint=(tmpint<<1)|(p_char-'0');
								read_srcpos++;p_char=Code2Char(*read_srcpos);cnt++;
							}
							tmpint*=4096;
						}else{
							return ERR_SYNTAX_ERROR;
						}
						errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						read_srcpos=JumpSpace(read_srcpos);
						p_char=Code2Char(*read_srcpos);
					}else if(p_char=='"'){
						read_srcpos++;
						p_char=Code2Char(*read_srcpos);
						mystrclear(&str);
						while((p_char!='"')&&(*read_srcpos!=0x0000)&&(*read_srcpos!=0x000D)){
							//ソースは最大でも一行100文字のため、
							//ここでSTR_LEN_MAX文字を超えることはない
							str.s[str.len]=Code2Char(*read_srcpos);
							str.len++;
							read_srcpos++;	
							p_char=Code2Char(*read_srcpos);
						}
						if(p_char=='"'){read_srcpos++;p_char=Code2Char(*read_srcpos);}
						errtmp=PushCalcStack(TYPE_STR_LIT,0,str,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
					}
					ProcessRemainingOperator();
					read_srcpos=JumpSpace(read_srcpos);
					if(Code2Char(*read_srcpos)==','){
						read_srcpos++;
					}else{
						read_initialized=false;
					}
					srcpos=JumpSpace(srcpos);
				}while(Code2Char(*srcpos)==',');
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_TMREAD:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=Char2Code('('))return ERR_SYNTAX_ERROR;
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_str(&str))return ERR_TYPE_MISMATCH;
				if(str.len!=8)return ERR_SYNTAX_ERROR;
				for(i=0;i<3;i++){					
					srcpos=JumpSpace(srcpos);
					if(*srcpos!=Char2Code(','))return ERR_MISSING_OPERAND;
					srcpos=JumpSpace(srcpos+1);
					srcpos=GetVarID(srcpos,&tmpint,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(inrange(str.s[i*3],'0','9')&&inrange(str.s[i*3+1],'0','9')&&((i==2)||(str.s[i*3+2]==':'))){
						tmpint=((str.s[i*3]-'0')*10+(str.s[i*3+1]-'0'))*4096;
					}else{
						return ERR_SYNTAX_ERROR;
					}
					errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
				}
				srcpos=JumpSpace(srcpos);
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_DTREAD:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=Char2Code('('))return ERR_SYNTAX_ERROR;
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				mystrclear(&str);
				if(!PopCalcStack_str(&str))return ERR_TYPE_MISMATCH;
				if(str.len!=10)return ERR_SYNTAX_ERROR;
				for(i=0;i<3;i++){					
					srcpos=JumpSpace(srcpos);
					if(*srcpos!=Char2Code(','))return ERR_MISSING_OPERAND;
					srcpos=JumpSpace(srcpos+1);
					srcpos=GetVarID(srcpos,&tmpint,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					switch(i){
					case 0:
						if(
							inrange(str.s[0],'0','9')&&inrange(str.s[1],'0','9')&&
							inrange(str.s[2],'0','9')&&inrange(str.s[3],'0','9')&&(str.s[4]=='/')
							){
								tmpint=((str.s[0]-'0')*1000+(str.s[1]-'0')*100+(str.s[2]-'0')*10+(str.s[3]-'0'))*4096;
						}else{
							return ERR_SYNTAX_ERROR;
						}
						break;
					case 1:
						if(inrange(str.s[5],'0','9')&&inrange(str.s[6],'0','9')&&(str.s[7]=='/')){
							tmpint=((str.s[5]-'0')*10+(str.s[6]-'0'))*4096;
						}else{
							return ERR_SYNTAX_ERROR;
						}
						break;
					case 2:
						if(inrange(str.s[8],'0','9')&&inrange(str.s[9],'0','9')){
							tmpint=((str.s[8]-'0')*10+(str.s[9]-'0'))*4096;
						}else{
							return ERR_SYNTAX_ERROR;
						}
						break;
					default:
						break;
					}
					errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
				}
				srcpos=JumpSpace(srcpos);
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_BGREAD:
				//BGREAD( レイヤー, x座標, y座標 ), CHR, PAL, H, V
				//具体的処理はEvalFormulaに投げる
				srcpos=JumpSpace(srcpos+1);
				errtmp=PushCalcStack(TYPE_FUNC,TOKEN_BGREAD,MYSTR_NULL,0);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(*srcpos!=Char2Code('('))return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code('('),MYSTR_NULL,0);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				for(i=0;i<=2;i++){
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(!PopCalcStack_int(&tmpint))return ERR_TYPE_MISMATCH;
					errtmp=PushCalcStack(TYPE_INT_LIT,tmpint,MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(*srcpos!=Char2Code(')')&&i==2){
						return ERR_SYNTAX_ERROR;
					}else{
						srcpos=JumpSpace(srcpos+1);
					}
					if(*srcpos!=Char2Code(',')&&i!=2)return ERR_MISSING_OPERAND;
					errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code(','),MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=JumpSpace(srcpos+1);
				}
				srcpos=JumpSpace(srcpos+1);
				for(i=0;i<=3;i++){
					srcpos=GetVarID(srcpos,&tmpint,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(Variable[tmpint].isStr)return ERR_TYPE_MISMATCH;
					if(i==3)break;
					if(*srcpos!=Char2Code(','))return ERR_MISSING_OPERAND;
					errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code(','),MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=JumpSpace(srcpos+1);
				}
				errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code(')'),MYSTR_NULL,0);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_IF:
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=PopCalcStack_int(&tmpint);
				if(!errtmp)return ERR_TYPE_MISMATCH;
				if(*srcpos==TOKEN_THEN){
					srcpos=JumpSpace(srcpos+1);
					if(*srcpos==TOKEN_LABEL || *srcpos==TOKEN_LABEL2){
						srcpos++;
						mystrclear(&str);
						for(i=0;i<8;i++){
							c=Code2Char(*srcpos);
							if((c==' ')||(*srcpos==0x000D)||(*srcpos==0x0000)||(*srcpos==':')||(*srcpos=='\''))break;
							if((c!='_')&&(!isalpha(c))&&(!isdigit(c))){
								return ERR_SYNTAX_ERROR;
							}
							str.s[i]=toupper(c);
							str.len++;
							srcpos++;
						}
						if(tmpint==0){
							cur_line++;
							if(cur_line>=srclinecount)return ERR_NO_ERROR;
							GOTOLINE(cur_line);
							break;
						}
						tmpint2=0;
						for(i=0;i<labelcount;i++){
							if(mystrcmp2(str,labellist_name[i])==0){
								cur_line=labellist_line[i];
								GOTOLINE(cur_line);
								tmpint2=1;
								break;
							}
						}
						if(tmpint2==0)return ERR_UNDEFINED_LABEL;
						break;
					}
					if(tmpint!=0){
						state=ST_NEW_STATEMENT;
						break;
					}else{
						cur_line++;
						if(cur_line>=srclinecount)return ERR_NO_ERROR;
						GOTOLINE(cur_line);
					}
				}else if(*srcpos==TOKEN_GOTO){
					if(tmpint==0){
						cur_line++;
						if(cur_line>=srclinecount)return ERR_NO_ERROR;
						GOTOLINE(cur_line);
					}
					//式が成り立っている場合は、そのままの状態で次の解析をさせる
				}else{
					return ERR_SYNTAX_ERROR;
				}
				break;
			case TOKEN_FOR:
				memset(ForGosub_s+ForGosub_sl,0x00,sizeof(ForGosub_s+ForGosub_sl));
				srcpos=JumpSpace(srcpos+1);
				srcpos=GetVarID(srcpos,&tmpint,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				ForGosub_s[ForGosub_sl].VarID=tmpint;
				if(Code2Char(*srcpos)!='=')return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				//代入処理ここまで
				if(*srcpos!=TOKEN_TO)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				ForGosub_s[ForGosub_sl].line=cur_line;
				ForGosub_s[ForGosub_sl].col=srcpos-input;
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_int(&tmpint))return ERR_SYNTAX_ERROR;
				if(*srcpos==Char2Code(':')){
					srcpos++;
					if((Variable[ForGosub_s[ForGosub_sl].VarID].value)>tmpint){
						srcpos=ForJump(srcpos,&errtmp);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
					}else{
						ForGosub_s[ForGosub_sl].step=4096;
						ForGosub_sl++;
						state=ST_NEW_STATEMENT;
						break;
					}
				}else if(*srcpos==Char2Code('\'') || *srcpos==0x0000 || *srcpos==0x000D){
					if((Variable[ForGosub_s[ForGosub_sl].VarID].value)>tmpint){
						srcpos=ForJump(srcpos,&errtmp);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						break;
					}else{
						ForGosub_s[ForGosub_sl].step=4096;
						ForGosub_sl++;
						cur_line++;
						if(cur_line>=srclinecount)return ERR_NO_ERROR;
						GOTOLINE(cur_line);
						break;
					}
				}
				tmpint3=tmpint;
				if(*srcpos!=TOKEN_STEP)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_int(&tmpint))return ERR_SYNTAX_ERROR;
				if(tmpint>0){
					if(Variable[ForGosub_s[ForGosub_sl].VarID].value>tmpint3){
						srcpos=ForJump(srcpos,&errtmp);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
					}
				}
				if(tmpint<0){
					if(Variable[ForGosub_s[ForGosub_sl].VarID].value<tmpint3){
						srcpos=ForJump(srcpos,&errtmp);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
					}
				}
				ForGosub_s[ForGosub_sl].step=tmpint;
				ForGosub_sl++;
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_NEXT:
				if(ForGosub_sl<=0)return ERR_NEXT_WITHOUT_FOR;
				ForGosub_sl--;
				mystrclear(&str);
				srcpos=JumpSpace(srcpos+1);
				while(isalpha(Code2Char(*srcpos))||(Code2Char(*srcpos)=='_')||(isdigit(Code2Char(*srcpos)))){
					if(str.len>=8)return ERR_STRING_TOO_LONG;
					str.s[str.len]=toupper(Code2Char(*srcpos));
					str.len++;
					srcpos++;
				}
				if(str.len!=0){
					tmpint=Str2VarID(str);
					if(tmpint==-1)return ERR_NEXT_WITHOUT_FOR;
					if(ForGosub_s[ForGosub_sl].VarID!=tmpint)return ERR_NEXT_WITHOUT_FOR;
				}
				tmpint=Variable[ForGosub_s[ForGosub_sl].VarID].value;
				tmpint+=ForGosub_s[ForGosub_sl].step;
				Variable[ForGosub_s[ForGosub_sl].VarID].value=tmpint;
				tmppos=srcpos;
				tmpline=cur_line;
				srcpos=input+ForGosub_s[ForGosub_sl].col;
				cur_line=ForGosub_s[ForGosub_sl].line;
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_int(&tmpint)){
					return ERR_SYNTAX_ERROR;
				}
				tmpint2=tmpint;
				if(*srcpos!=TOKEN_STEP){
					if(Variable[ForGosub_s[ForGosub_sl].VarID].value>tmpint){
						cur_line=tmpline;
						GOTOLINE(cur_line);
						srcpos=tmppos;	
					}else{
						ForGosub_s[ForGosub_sl].step=4096;
						ForGosub_sl++;
					}
					state=ST_NEW_STATEMENT;
					break;
				}
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_int(&tmpint))return ERR_SYNTAX_ERROR;
				if(tmpint>0){
					if(Variable[ForGosub_s[ForGosub_sl].VarID].value>tmpint2){
						srcpos=tmppos+1;
						cur_line=tmpline;
						state=ST_NEW_STATEMENT;
						break;
					}
				}
				if(tmpint<0){
					if(Variable[ForGosub_s[ForGosub_sl].VarID].value<tmpint2){
						srcpos=tmppos+1;
						cur_line=tmpline;
						state=ST_NEW_STATEMENT;
						break;
					}
				}
				ForGosub_s[ForGosub_sl].step=tmpint;
				ForGosub_sl++;
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_INPUT:
				//TODO:複数変数の代入
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos==Char2Code('"')){
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('('),MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code(')'),MYSTR_NULL,0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('+'),MYSTR_NULL,2);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=PushCalcStack(TYPE_STR_LIT,0,str2mystr2("?"),0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(*srcpos!=Char2Code(';'))return ERR_SYNTAX_ERROR;
					srcpos=JumpSpace(srcpos+1);
				}else{
					errtmp=PushCalcStack(TYPE_STR_LIT,0,str2mystr2("?"),0);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
				}
				memset(tmpstr,0x00,sizeof(tmpstr));
				if(!PopCalcStack_str(&str))return ERR_UNDEFINED;
				tmpint3=0;
				do{
					if(tmpint3==1)Print2Console(str2mystr2("?REDO FROM START"),0);
					Print2Console(str2mystr2(tmpstr),0);
					InputLine(&str);
					if(breakflag==1)break;
					if(tmpint3==0)srcpos=GetVarID(srcpos,&tmpint,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(Variable[tmpint].isStr){
						errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						errtmp=PushCalcStack(TYPE_STR_LIT,0,str,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						break;
					}else{
						if(isintliteral(str)){
							mystr2str2(str,tmpstr);
							tmpint2=(int32_t)(atof(tmpstr)*4096.0);//本来はVAL()を内部的に使用
							errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							errtmp=PushCalcStack(TYPE_INT_LIT,(int32_t)tmpint2,MYSTR_NULL,0);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							break;
						}
					}
					tmpint3=1;
				}while(1);
				if(breakflag==1)break;
				srcpos=JumpSpace(srcpos);
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_LINPUT:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos==Char2Code('"')){
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(*srcpos!=Char2Code(';'))return ERR_SYNTAX_ERROR;
					srcpos=JumpSpace(srcpos+1);
				}
				srcpos=GetVarID(srcpos,&tmpint,&errtmp);
				if(!Variable[tmpint].isStr)return ERR_TYPE_MISMATCH;
				errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				mystrclear(&str);
				InputLine(&str);
				errtmp=PushCalcStack(TYPE_STR_LIT,0,str,0);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_LOAD:
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				errtmp=PopCalcStack_str(&str);
				mystr2str2(str,tmpstr);
				if(!errtmp)return ERR_SYNTAX_ERROR;
				c2=strchr(tmpstr,':');
				if(c2==NULL){
					if(strlen(tmpstr)>8)return ERR_ILLEGAL_FUNCTION_CALL;
					for(i=0;tmpstr[i]!=0;i++){
						if((!isupper(tmpstr[i]))&&(!isdigit(tmpstr[i]))&&(tmpstr[i]!='_'))return ERR_ILLEGAL_FUNCTION_CALL;
					}
					errtmp=LoadPResource("PRG",tmpstr);
					if(errtmp!=ERR_NO_ERROR)return errtmp;						   
					if(Psys_SYSBEEP)PlaySoundMem(SHandleBEEP[46],DX_PLAYTYPE_BACK);
					break;
				}else{
					for(i=0;tmpstr[i]!=':';i++){
						if((i>6)||((!isupper(tmpstr[i]))&&(!isdigit(tmpstr[i]))))return ERR_ILLEGAL_RESOURCE_TYPE;
					}
					tmpint=i;
					for(i++;tmpstr[i]!=0;i++){
						if((i>8)||((!isupper(tmpstr[i]))&&(!isdigit(tmpstr[i]))))return ERR_ILLEGAL_FUNCTION_CALL;
					}
					memcpy(tmpstr2,tmpstr,tmpint);
					memcpy(tmpstr3,&tmpstr[tmpint+1],i-tmpint-1);
					errtmp=LoadPResource(tmpstr2,tmpstr3);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					if(Psys_SYSBEEP)PlaySoundMem(SHandleBEEP[46],DX_PLAYTYPE_BACK);
				}
				break;
			case TOKEN_RUN:
				if(!interactive_flag)return ERR_ILLEGAL_FUNCTION_CALL;
				*runflag=1;
				return ERR_NO_ERROR;
				break;
			case TOKEN_NEW:
				if(!interactive_flag)return ERR_ILLEGAL_FUNCTION_CALL;
				memset(srcline_begin_token_pos,0x0000,sizeof(srcline_begin_token_pos));
				memset(srcline_token_count,0x0000,sizeof(srcline_token_count));
				srclinecount=0;
				memset(source_ptr,0x0000,sizeof(source_ptr));
				memset(translated_source,0x0000,sizeof(translated_source));
				return ERR_NO_ERROR;
				break;
			case TOKEN_GOTO:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=TOKEN_LABEL && *srcpos!=TOKEN_LABEL2)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				srcpos=GetLabelName(srcpos,tmpstr,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				tmpint=0;
				for(i=0;i<labelcount;i++){
					if(strcmp(labellist_name[i],tmpstr)==0){
						cur_line=labellist_line[i];
						GOTOLINE(cur_line);
						tmpint=1;
						break;
					}
				}
				if(tmpint==0)return ERR_UNDEFINED_LABEL;
				break;
			case TOKEN_GOSUB:
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=TOKEN_LABEL && *srcpos!=TOKEN_LABEL2)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				memset(tmpstr,0x00,sizeof(tmpstr));
				for(i=0;i<8;i++){
					c=Code2Char(*srcpos);
					if((c==' ')||(*srcpos==0x000D)||(*srcpos==0x0000)||(c==':')||(c=='\''))break;
					if((c!='_')&&(!isalpha(c))&&(!isdigit(c))){
						return ERR_SYNTAX_ERROR;
					}
					tmpstr[i]=toupper(c);
					srcpos++;
				}
				tmpint=0;
				for(i=0;i<labelcount;i++){
					if(strcmp(labellist_name[i],tmpstr)==0){
						ForGosub_s[ForGosub_sl].col=srcpos-input;
						ForGosub_s[ForGosub_sl].line=cur_line;
						cur_line=labellist_line[i];
						GOTOLINE(cur_line);
						ForGosub_sl++;
						tmpint=1;
						break;
					}
				}
				if(tmpint==0)return ERR_UNDEFINED_LABEL;
				break;
			case TOKEN_ON:
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				if(!PopCalcStack_int(&tmpint))return ERR_SYNTAX_ERROR;
				tmpint=FloorInt(tmpint);
				if(tmpint<0){
					while((*srcpos!=0x0000)&&(*srcpos!=0x000D)&&(c!=':')&&(c!='\'')){
						srcpos++;
						c=Code2Char(*srcpos);
					}
					break;
				}
				srcpos=JumpSpace(srcpos);
				tmpint2=0;
				if(*srcpos==TOKEN_GOTO)tmpint2=1;
				if(*srcpos==TOKEN_GOSUB){
					tmpint2=2;
				}
				if(tmpint2==0)return ERR_SYNTAX_ERROR;
				if(tmpint>0){
					for(i=0;i<tmpint;i++){
						if(i!=0)srcpos++;
						while(1){
							c=Code2Char(*srcpos);
							if(c==',')break;
							if((*srcpos==0x0000)||(*srcpos==0x000D)||(c==':')||(c=='\'')){
								tmpint2=0;
								break;
							}
							srcpos++;
						}
					}
				}
				if(tmpint2==0)break;
				srcpos=JumpSpace(srcpos+1);
				if(*srcpos!=TOKEN_LABEL && *srcpos!=TOKEN_LABEL2)return ERR_SYNTAX_ERROR;
				srcpos=JumpSpace(srcpos+1);
				memset(tmpstr,0x00,sizeof(tmpstr));
				for(i=0;i<8;i++){
					c=Code2Char(*srcpos);
					if((c==' ')||(*srcpos==0x000D)||(*srcpos==0x0000)||(c==':')||(c=='\'')||(c==','))break;
					if((c!='_')&&(!isalnum(c))){
						return ERR_SYNTAX_ERROR;
					}
					tmpstr[i]=toupper(c);
					srcpos++;
				}
				tmpint=0;
				for(i=0;i<labelcount;i++){
					if(strcmp(labellist_name[i],tmpstr)==0){
						if(tmpint2==1){//GOTO
							cur_line=labellist_line[i];
							GOTOLINE(cur_line);
							tmpint=1;
							break;
						}else if(tmpint2==2){//GOSUB
							while(1){
								if(*srcpos==0x000D || *srcpos==0x0000 || *srcpos==Char2Code(':'))break;
								if(Code2Char(*srcpos)==0 && (*srcpos!=TOKEN_LABEL && *srcpos!=TOKEN_LABEL2))break;
								srcpos++;
							}
							ForGosub_s[ForGosub_sl].col=srcpos-input;
							ForGosub_s[ForGosub_sl].line=cur_line;
							cur_line=labellist_line[i];
							GOTOLINE(cur_line);
							ForGosub_sl++;
							tmpint=1;
							state=ST_NEW_STATEMENT;
							break;
						}
					}
				}
				if(tmpint==0)return ERR_UNDEFINED_LABEL;
				break;
			case TOKEN_RETURN:
				if(ForGosub_sl<=0)return ERR_RETURN_WITHOUT_GOSUB;
				ForGosub_sl--;
				srcpos=input+ForGosub_s[ForGosub_sl].col;
				cur_line=ForGosub_s[ForGosub_sl].line;
				srcpos=JumpSpace(srcpos);
				state=ST_NEW_STATEMENT;
				break;
			case TOKEN_LABEL:case TOKEN_LABEL2:
				cur_line++;
				if(cur_line>=srclinecount)return ERR_NO_ERROR;
				GOTOLINE(cur_line);
				break;
			case TOKEN_END:
				return ERR_NO_ERROR;
			case TOKEN_STOP:
				breakflag=1;
				return ERR_NO_ERROR;
			default:
				if(isInstruction(t)){
					if(isNoArgInstruction(t)){
						errtmp=PushCalcStack(TYPE_FUNC,t,MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						errtmp=PushCalcStack(TYPE_VOID,0,MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						ProcessRemainingOperator();
						srcpos++;
						state=ST_NEW_STATEMENT;
						break;
					}else{
						errtmp=PushCalcStack(TYPE_FUNC,t,MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						errtmp=PushCalcStack(TYPE_FUNC,Char2Code('('),MYSTR_NULL,0);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						srcpos=JumpSpace(srcpos+1);
						if((*srcpos==0x0000)||(Code2Char(*srcpos)==':')){
							errtmp=PushCalcStack(TYPE_VOID,0,MYSTR_NULL,0);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							errtmp=PushCalcStack(TYPE_FUNC,Char2Code(')'),MYSTR_NULL,0);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							state=ST_NEW_STATEMENT;
							break;
						}
						srcpos=ReadFormula(srcpos,&errtmp);
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						argcount=1;
						while(Code2Char(*srcpos)==','){
							errtmp=PushCalcStack(TYPE_SPECIAL,Char2Code(','),MYSTR_NULL,0);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							srcpos=JumpSpace(srcpos+1);
							srcpos=ReadFormula(srcpos,&errtmp);
							if(errtmp!=ERR_NO_ERROR)return errtmp;
							argcount++;
						}
						errtmp=PushCalcStack(TYPE_FUNC,Char2Code(')'),MYSTR_NULL,argcount);
						if(errtmp!=ERR_NO_ERROR)return ERR_UNDEFINED;
						errtmp=ProcessRemainingOperator();
						if(errtmp!=ERR_NO_ERROR)return errtmp;
						state=ST_NEW_STATEMENT;
						break;
					}
				}else if((GetSystemVariableType(*srcpos)==2)||(GetSystemVariableType(*srcpos)==4)){
					if(GetSystemVariableType(*srcpos)==2){
						errtmp=PushCalcStack(TYPE_INT_SYSVAR,*srcpos,MYSTR_NULL,0);
					}else{
						//MEM$
						errtmp=PushCalcStack(TYPE_STR_SYSVAR,*srcpos,MYSTR_NULL,0);
					}
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=JumpSpace(srcpos+1);
					if(Code2Char(*srcpos)!='=')return ERR_SYNTAX_ERROR;
					errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					srcpos=JumpSpace(srcpos+1);
					srcpos=ReadFormula(srcpos,&errtmp);
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					errtmp=ProcessRemainingOperator();
					if(errtmp!=ERR_NO_ERROR)return errtmp;
					state=ST_NEW_STATEMENT;
					break;		
				}else if(*srcpos==0x0000){
					if(state==ST_SUBSTITUTION_NAME)return ERR_SYNTAX_ERROR;
					return ERR_NO_ERROR;
				}else{
					//error
					return ERR_SYNTAX_ERROR;
				}
				break;
			}
		}else{
			//Char
			switch(state){
			case ST_LINE_BEGIN:
				if(c=='\''){
					cur_line++;
					if(cur_line>=srclinecount)return ERR_NO_ERROR;
					GOTOLINE(cur_line);
					srcpos--;
				}else if(c=='@'){
					cur_line++;
					if(cur_line>=srclinecount)return ERR_NO_ERROR;
					GOTOLINE(cur_line);
					srcpos--;
				}else if(c=='?'){
					state=ST_PRINT;
					//代入
				}else if(isalpha(c)||(c=='_')){
					memset(tmpstr,0x00,sizeof(tmpstr));
					tmpstr_p=0;
					tmpstr[0]=toupper(c);//一文字目記録
					tmpstr_p++;
					state=ST_SUBSTITUTION_NAME;
				}else if((c==' ')||(c==':')){
					//NOP
				}else if(c==0){
					return ERR_NO_ERROR;
				}else{
					if(*srcpos==0x0D){
						cur_line++;
						if(cur_line>=srclinecount)return ERR_NO_ERROR;
						GOTOLINE(cur_line);
						break;
					}
					printf("srcpos=%X='%c'\n",t,c);
					return ERR_SYNTAX_ERROR;
				}
				srcpos++;
				break;
			case ST_NEW_STATEMENT:
				if(c=='\''){
					cur_line++;
					if(cur_line>=srclinecount)return ERR_NO_ERROR;
					GOTOLINE(cur_line);
					srcpos--;
				}else if(c=='@'){
					cur_line++;
					if(cur_line>=srclinecount)return ERR_NO_ERROR;
					GOTOLINE(cur_line);
					srcpos--;
				}else if(c=='?'){
					state=ST_PRINT;
					//代入
				}else if(isalpha(c)||(c=='_')){
					memset(tmpstr,0x00,sizeof(tmpstr));
					tmpstr_p=0;
					tmpstr[0]=toupper(c);//一文字目記録
					tmpstr_p++;
					state=ST_SUBSTITUTION_NAME;
				}else if((c==' ')||(c==':')){
					//NOP
				}else if(c==0){
					//正常終了
					return ERR_NO_ERROR;
				}else{
					if(*srcpos==0x0D){
						cur_line++;
						if(cur_line>=srclinecount)return ERR_NO_ERROR;
						GOTOLINE(cur_line);
						break;
					}
					printf("srcpos=%X='%c'\n",t,c);
					return ERR_SYNTAX_ERROR;
				}
				srcpos++;
				break;
			case ST_SUBSTITUTION_NAME:
				srcpos--;
				srcpos=GetVarID(srcpos,&tmpint,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos);
				if(Code2Char(*srcpos)!='=')return ERR_SYNTAX_ERROR;
				errtmp=PushCalcStack(TYPE_FUNC,Char2Code('='),MYSTR_NULL,2);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				srcpos=JumpSpace(srcpos+1);
				srcpos=ReadFormula(srcpos,&errtmp);
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				errtmp=ProcessRemainingOperator();
				if(errtmp!=ERR_NO_ERROR)return errtmp;
				state=ST_NEW_STATEMENT;
				break;		
			default:
				if(*srcpos==0x0D){
					cur_line++;
					if(cur_line>=srclinecount)return ERR_NO_ERROR;
					GOTOLINE(cur_line);
					break;
				}
				srcpos++;
				break;
			}
		}
		tmpint=ProcessFrame();
		if(log_en2)printf("%s/",TokenCode2Str(*srcpos).s);
		if(!tmpint)return ERR_UNDEFINED;
		if(breakflag)return ERR_NO_ERROR;
	}
	ProcessRemainingOperator(); 
	return ERR_NO_ERROR;
}

int NewVar(st name){
	int type=0;
	if((Psys_FREEVAR/4096)<=0)return -1;
	Variable[VAR_MAX-Psys_FREEVAR/4096].name=name;
	if(calc_sl>=CALC_S_MAX)return -1;
	if(name.s[name.len-1]=='$' || name.s[name.len-2]=='$'){
		Variable[VAR_MAX-FloorInt(Psys_FREEVAR)].isStr=true;
		Variable[VAR_MAX-FloorInt(Psys_FREEVAR)].isDim=0;
		type=TYPE_STR_VAR;
	}else{
		Variable[VAR_MAX-FloorInt(Psys_FREEVAR)].isStr=false;
		Variable[VAR_MAX-FloorInt(Psys_FREEVAR)].isDim=0;
		type=TYPE_INT_VAR;
	}
	Psys_FREEVAR-=4096;
	return VAR_MAX-Psys_FREEVAR/4096-1;
}



#ifdef __cplusplus
}
#endif