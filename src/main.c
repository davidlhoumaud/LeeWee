#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <regex.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>

#include <sys/stat.h>
#include <sys/types.h>

#define LG_MAX 40960


#include "regex.c"
#include "regex_const.c"
#include "str.c"
#include "console.c"
#include "markers.c"
#include "functions.c"
#include "variables.c"


#include "tokens.h"

void getVarsRequest(char* VARSQUERY) {
    int nameget=0;
    int i;
    if (VARSQUERY != NULL) {
        char *var_name=(char *)malloc (sizeof (char) * LG_MAX);
        char *var_value=(char *)malloc (sizeof (char) * LG_MAX);
        memset (var_name, 0, strlen (var_name));
        memset (var_value, 0, strlen (var_value));
        for(i=0;i<=strlen(VARSQUERY);i++) {
            if (VARSQUERY[i]!='\0') {
                if (VARSQUERY[i]=='=') {
                    nameget=1;
                } else if (VARSQUERY[i]=='&') {
                    nameget=0;
                    libererVariable(mes_variables, var_name);
                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                    memset (var_name, 0, strlen (var_name));
                    memset (var_value, 0, strlen (var_value));
                } else {
                    if (nameget==0) {
                        strncat(var_name, &VARSQUERY[i], 1);
                    } else {
                        strncat(var_value, &VARSQUERY[i], 1);
                    }
                }
            } else {
                nameget=0;
                libererVariable(mes_variables, var_name);
                mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                memset (var_name, 0, strlen (var_name));
                memset (var_value, 0, strlen (var_value));
            }
        }
    }
}
void dbg(char *msg) {
    printf("%s", msg);
    exit(0);
}

void dbgint(int msg) {
    printf("%d", msg);
    exit(0);
}

int str2int(char *msg) {
    int OUT=0;
    //int ind;
    if ( (regex_match(msg,"^[ 0-9]*$")==0) || (regex_match(msg,"^-+[ 0-9]*$")==0) || (regex_match(msg,"^[+]+[ 0-9]*$")==0) ) {
        OUT=(int) strtol(str_strip(msg), (char **)NULL, 10);
/*        for (ind = 0; ind < strlen(msg); ind++) {*/
/*            OUT=OUT+(int)msg[ind];*/
/*        }*/
    } else {
        OUT=(int)strlen(msg);
    }
    return OUT;
}

int update_session_file(char *fInName, char *cmp, char *out, int multiline) 
{ 
    char ligne[LG_MAX]; 
    FILE * fIn; 
    FILE * fOut; 
    int session_var=0;
    int oneline_var=0;
    char *fOutName = (char *)malloc(sizeof(char) * LG_MAX);
    sprintf(fOutName, "%s_tmp", fInName);
    if ((fIn = fopen(fInName, "r+")) == NULL) return -1; 
  
    if ((fOut = fopen(fOutName, "w+")) == NULL) { 
        fclose(fIn);
        return -1;
    } 
    while (fgets(ligne, sizeof ligne, fIn)) { 
        if (str_istr(ligne, cmp)>-1 && multiline==0) {
            oneline_var=1;
        }
        if (str_istr(ligne, cmp)<0 && session_var==0) {
            if (regex_match(ligne,"^\\$.*=.*$")==0 || oneline_var==0) {
                oneline_var=0;
                fputs(ligne, fOut); 
                if (str_istr(ligne, "\n")<0 ) fputc('\n',fOut);
            }
        } else if (session_var==1 && (regex_match(ligne,"^\\$.*=.*$")==0)) {
            fputs(ligne, fOut); 
            if (str_istr(ligne, "\n")<0 ) fputc('\n',fOut);
            session_var=0;
        } else {
            if (multiline==1) session_var=1;
        }
    }
    fputs(out,fOut);
    fputc('\n',fOut);
    //fclose(fIn);
    //fclose(fOut);
    rename(fOutName, fInName); 
    fclose(fOut);
    return 0; 
}

int uploadFile() {
    int content_length;
    char *ligne_courante_tmp = (char *)malloc (sizeof(char) * LG_MAX);
    char *var_name=(char *)malloc (sizeof (char) * LG_MAX);
    char *var_value=(char *)malloc (sizeof (char) * LG_MAX);
    char *uploadtmp=(char *)malloc (sizeof (char) * LG_MAX);
    char *tmp_tmp = (char *)malloc(sizeof(char) * LG_MAX);
    char *tmp_tmp_tmp = (char *)malloc(sizeof(char) * LG_MAX);
    content_length = atoi(getenv("CONTENT_LENGTH")); 
    if(content_length > 0) {
        FILE* fileupload = NULL;
            int ni; 
            int nc;
            time_t timestamp; 
            struct tm * t; 
            timestamp = time(NULL); 
            t = localtime(&timestamp); 
            sprintf(uploadtmp,".%04u%02u%02u%02u%02u%02u", 1900 + t->tm_year, t->tm_mon,t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

            //on compte le nombre de ligne
            FILE *ftmp = fopen(uploadtmp, "wb");
            for ( ni = 0; ni < content_length && (nc = getchar()) != EOF ; ni++ ) {
                fputc ( nc, ftmp );
            }
            fclose(ftmp);
            ftmp = fopen (uploadtmp, "rb");
            int skipline=0;
            int skipfile=0;
            while(fgets(ligne_courante_tmp, LG_MAX, ftmp) != NULL) {
                
                if (str_istr(ligne_courante_tmp,"WebKitFormBoundary")!= -1) continue;
                if (str_istr(ligne_courante_tmp,"Content-Disposition")!=-1) {
                    skipfile=0;
                    //on stock la variable pour le name_filename
                    tmp_tmp=regex_const(ligne_courante_tmp,TOKEN_VAR_NAME_FILEUPLOAD_1);
                    var_name=str_replace(tmp_tmp,0,8,"");
                    strcpy(tmp_tmp_tmp,var_name);
                    strcat(var_name, "_filename");
                    tmp_tmp=regex_const(ligne_courante_tmp,TOKEN_VAR_FILENAME_FILEUPLOAD_1);
                    var_value=str_replace(tmp_tmp,0,12,"");
                    var_value=str_replace(var_value,strlen(var_value)-1, 1,"");

                    if (strlen(var_value)<=0) {skipfile=1;continue;}
                    libererVariable(mes_variables, var_name);
                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                    if (fileupload != NULL) {
                        //fputc ( '\0', fileupload );
                        fclose(fileupload);
                    }
                    fileupload = fopen(var_value, "w+b");
                    
                    continue;
                }
                if (str_istr(ligne_courante_tmp,"Content-Type")!=-1 && skipfile==0) {
                    //on stock la variable pour le name_type
                    strcpy(var_name,tmp_tmp_tmp);
                    strcat(var_name, "_type");
                    tmp_tmp=regex_const(ligne_courante_tmp,TOKEN_VAR_TYPE_FILEUPLOAD_1);
                    var_value=str_replace(tmp_tmp,0,14,"");
                    libererVariable(mes_variables, var_name);
                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                    skipline=1;
                    memset (var_name, 0, strlen (var_name));
                    memset (var_value, 0, strlen (var_value));
                    continue;
                }
                if (skipline==0 && skipfile==0) {
                    for ( ni = 0; ni < content_length && (nc = ligne_courante_tmp[ni]) != '\n' ; ni++ ) {
                        fputc ( nc, fileupload );
                    }
                    fputc ( '\n', fileupload );
                } else skipline=0;
                
            }
            if (fileupload != NULL) fclose(fileupload);
            if (fileupload != NULL) fclose(ftmp);
            remove(uploadtmp);
            return 1;
        
    } else return 0;
}


//Analyse d'un script
int parseScript(FILE *SCRIPT, int pos_html) {
    if (SCRIPT) {
        char c;
        int i;

        char *ligne_courante = (char *)malloc (sizeof(char) * LG_MAX);
        //char *ligne_courante = (char*)calloc(U_MAX_BYTES + 1, sizeof(char)); 
        int pos_html2=pos_html;
        int ctmp=0;
        int multiline=0;
        int pstr=-1;
        char *tmp_var_name=(char *)malloc (sizeof (char) * LG_MAX);
        char *tmp = (char *)malloc(sizeof(char) * LG_MAX);
        char *tmp_tmp = (char *)malloc(sizeof(char) * LG_MAX);
        char *tmp_tmp_tmp = (char *)malloc(sizeof(char) * LG_MAX);
        
        int display_var=0;
        
        char *var_name=(char *)malloc (sizeof (char) * LG_MAX);
        
        char *var_value=(char *)malloc (sizeof (char) * LG_MAX);

        int session_var=0;
        long tmppos ;
        long currentpos ;
        int condition=0;
        int condition_count=-1;
        int function_count=-1;
        int condition_etat=0;
        char **tableau_value = NULL;
        int FUNCTION[LG_MAX];
        int CONDITION[LG_MAX];
        int CONDITION_POS[LG_MAX];
        int CONDITION_TYPE[LG_MAX]; //0=if 1=while 2=for 3=function
        while(fgets(ligne_courante, LG_MAX, SCRIPT) != NULL) {
            if (multiline==0) memset (tmp, 0, LG_MAX);
            currentpos=ftell(SCRIPT)-strlen(ligne_courante);
            for(i=0;i<=strlen(ligne_courante);i++) {
                c=ligne_courante[i];
                switch(c) {
                    case '\n': 
                        if (display_var<2) display_var=0;
                        //si ça ne commence pas par # (commentaire) alors...
                        if ( ((tmp[0] != '#' && multiline==0) || (multiline==1)) && (tmp[0] != 0) ) {
                            /*si la ligne vaut $var=<? cmd ?>*/
                            if ((regex_match_const(tmp,TOKEN_VAR_1)==0 || regex_match_const(tmp,TOKEN_VAR_2)==0) && (multiline==0) ){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    var_name=regex_const(regex_const(tmp,TOKEN_VAR_NAME_1),TOKEN_VAR_NAME_2);
                                    tmp_tmp=regex(tmp,"(<\\?.*\\?>$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,2,"");
                                    pstr=str_istr(tmp_tmp,"?>");
                                    tmp_tmp=str_replace(tmp_tmp,pstr,2,"");
                                    var_value=shell(tmp_tmp);
                                    libererVariable(mes_variables, var_name);
                                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                                }
                            /*si la ligne vaut s$var=<? cmd ?>*/
                            } else if ((regex_match_const(tmp,TOKEN_VAR_SESSION_1)==0 || regex_match_const(tmp,TOKEN_VAR_SESSION_2)==0) && (multiline==0) ){
                                
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    var_name=regex_const(regex_const(tmp,TOKEN_VAR_SESSION_NAME_1),TOKEN_VAR_NAME_2);
                                    tmp_tmp=regex(tmp,"(<\\?.*\\?>$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,2,"");
                                    pstr=str_istr(tmp_tmp,"?>");
                                    tmp_tmp=str_replace(tmp_tmp,pstr,2,"");
                                    var_value=shell(tmp_tmp);
                                    libererVariable(mes_variables, var_name);
                                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                                    
                                    sprintf(tmp_tmp_tmp, "$%s", var_name);
                                    update_session_file(afficherVariable(mes_variables, "LW_SESSION"), tmp_tmp_tmp, str_replace(tmp, 0, 2, "$"), 0);
                                }
                            /*si la ligne vaut $var=<? cmd*/
                            } else if ((regex_match_const(tmp,TOKEN_VAR_START_1)==0 || regex_match_const(tmp,TOKEN_VAR_START_2)==0) && (multiline==0)){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    tmp_tmp=regex(tmp,"(<\\?.*$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,2,"");
                                    //printf("%s\033[0m\n",tmp_tmp);
                                    strncat(tmp, "\n", 1);
                                    multiline=1;
                                    session_var=0;
                                }
                            /*si la ligne vaut s$var=<? cmd*/
                            } else if ((regex_match_const(tmp,TOKEN_VAR_SESSION_START_1)==0 || regex_match_const(tmp,TOKEN_VAR_SESSION_START_2)==0) && (multiline==0)){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    tmp_tmp=regex(tmp,"(<\\?.*$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,2,"");
                                    //printf("%s\033[0m\n",tmp_tmp);
                                    strncat(tmp, "\n", 1);
                                    multiline=1;
                                    session_var=1;
                                }

                            /*si la ligne vaut cmd ?>*/
                            } else if (regex_match_const(tmp,TOKEN_VAR_END_1_2)==0){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    if (regex_match(tmp,"^.*\\?>$")==0 ) {
                                        if (session_var==1) var_name=regex_const(regex_const(tmp,TOKEN_VAR_SESSION_NAME_1),TOKEN_VAR_NAME_2);
                                        else var_name=regex_const(regex_const(tmp,TOKEN_VAR_NAME_1),TOKEN_VAR_NAME_2);
                                        tmp_tmp=regex(tmp,"(.*\\?>$)");
                                        pstr=str_istr(tmp_tmp,"?>");
                                        tmp_tmp=str_replace(tmp_tmp,pstr,2,"");
                                        pstr=str_istr(tmp_tmp,"<?");
                                        tmp_tmp=str_replace(tmp_tmp,pstr,2,"");
                                        strcpy(tmp_tmp_tmp,tmp_tmp);
                                        tableau_value=str_split(tmp_tmp_tmp, "=");
                                        pstr=str_istr(tmp_tmp, tableau_value[0]);
                                        tmp_tmp=str_replace(tmp_tmp,pstr,strlen(tableau_value[0])+1,"");
                                        var_value=shell(tmp_tmp);
                                        libererVariable(mes_variables, var_name);
                                        mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                                        sprintf(tmp_tmp_tmp, "$%s", var_name);
                                        if (session_var==1) {
                                            update_session_file(afficherVariable(mes_variables, "LW_SESSION"), tmp_tmp_tmp, str_replace(tmp, 0, 2, "$"), 1);
                                        }
                                        session_var=0;
                                    }
                                    multiline=0;
                                }
                            } else if (multiline==1){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    strncat(tmp, "\n", 1);
                                }

                            /*si la ligne vaut $var=str*/
                            } else if ((regex_match_const(tmp,TOKEN_VAR_3)==0) && (multiline==0)){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    var_name=regex_const(regex_const(tmp,TOKEN_VAR_NAME_1),TOKEN_VAR_NAME_2);
                                    tmp_tmp=regex(tmp,"(=.*$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,1,"");
                                    var_value=tmp_tmp;
                                   //printf("##%s == %s\n",var_name,var_value);
                                    libererVariable(mes_variables, var_name);
                                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                                }
                            /*si la ligne vaut s$var=str*/
                            } else if ((regex_match_const(tmp,TOKEN_VAR_SESSION_3)==0) && (multiline==0)){
                                if ((condition==1 && condition_etat >0) || (condition==0)) {
                                    var_name=regex_const(regex_const(tmp,TOKEN_VAR_SESSION_NAME_1),TOKEN_VAR_NAME_2);
                                    tmp_tmp=regex(tmp,"(=.*$)");
                                    tmp_tmp=str_replace(tmp_tmp,0,1,"");
                                    var_value=tmp_tmp;
                                   //printf("##%s == %s\n",var_name,var_value);
                                    libererVariable(mes_variables, var_name);
                                    mes_variables=ajouterVariable(mes_variables, var_name, var_value);
                                    
                                    sprintf(tmp_tmp_tmp, "$%s", var_name);
                                    update_session_file(afficherVariable(mes_variables, "LW_SESSION"), tmp_tmp_tmp, str_replace(tmp, 0, 2, "$"), 0);
                                }
/*************************************FUNCTION*****************************/
                            } else if (regex_match_const(tmp,TOKEN_FUNCTION_1)==0 || regex_match_const(tmp,TOKEN_FUNCTION_2)==0 || regex_match_const(tmp,TOKEN_FUNCTION_3)==0 || regex_match_const(tmp,TOKEN_FUNCTION_4)==0) {
                                condition=1;
                                condition_etat=0;
                                condition_count++; 
                                CONDITION_TYPE[condition_count]=3;
                                CONDITION[condition_count]=0;
                                tmp_var_name=regex_const(tmp,TOKEN_FUNCTION_NAME_1);
                                

                                //var_name=str_replace(tmp_tmp,0,-1,"");
                                libererFonction(mes_fonctions, tmp_var_name);
                                tmppos = ftell (SCRIPT);
                                mes_fonctions=ajouterFonction(mes_fonctions, tmp_var_name, tmppos);
/*************************************CONDITION*****************************/
/*****************************************IF********************************/
                            } else if (regex_match_const(tmp,TOKEN_IF_1)==0 || regex_match_const(tmp,TOKEN_IF_2)==0 || regex_match_const(tmp,TOKEN_IF_3)==0 || regex_match_const(tmp,TOKEN_IF_4)==0) {
                                //condition = 1 alors condition activé
                                // condition_etat= 1 alors condition vraie
                                
                                condition=1;
                                condition_count++; 
                                CONDITION_TYPE[condition_count]=0;
                                if (condition_count>0 && CONDITION[condition_count-1]==0){
                                    condition_etat=0;
                                    CONDITION_POS[condition_count]=currentpos;
                                    CONDITION[condition_count]=0;
                                } else if (regex_match(tmp,"\\(.*!=.*\\)")==0){ //si condition de type pas égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*!=)"),strlen(regex(tmp,"(\\(.*!=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(!=.*\\))"),strlen(regex(tmp,"(!=.*\\))"))-1,1,""),0,2,"");
                                    if (strcmp(var_name, var_value)==0) {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    } else {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    }
                                } else if (regex_match(tmp,"\\(.*==.*\\)")==0){ //si condition de type égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*==)"),strlen(regex(tmp,"(\\(.*==)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(==.*\\))"),strlen(regex(tmp,"(==.*\\))"))-1,1,""),0,2,"");
                                    if (strcmp(var_name, var_value)==0) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*>=.*\\)")==0){ //si condition de type plus grand ou égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*>=)"),strlen(regex(tmp,"(\\(.*>=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(>=.*\\))"),strlen(regex(tmp,"(>=.*\\))"))-1,1,""),0,2,"");
                                    if (str2int(var_name)>=str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*>.*\\)")==0){ //si condition de type plus grand
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*>)"),strlen(regex(tmp,"(\\(.*>)"))-1,1,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(>.*\\))"),strlen(regex(tmp,"(>.*\\))"))-1,1,""),0,1,"");
                                    if (str2int(var_name)>str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*<=.*\\)")==0){ //si condition de type plus petit ou égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*<=)"),strlen(regex(tmp,"(\\(.*<=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(<=.*\\))"),strlen(regex(tmp,"(<=.*\\))"))-1,1,""),0,2,"");
                                    if (str2int(var_name)<=str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*<.*\\)")==0){ //si condition de type plus petit
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*<)"),strlen(regex(tmp,"(\\(.*<)"))-1,1,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(<.*\\))"),strlen(regex(tmp,"(<.*\\))"))-1,1,""),0,1,"");
                                    if (str2int(var_name)<str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                }
/*****************************************ELSE********************************/
                            } else if (regex_match_const(tmp,TOKEN_ELSE_1)==0 || regex_match_const(tmp,TOKEN_ELSE_2)==0 || regex_match_const(tmp,TOKEN_ELSE_3)==0 || regex_match_const(tmp,TOKEN_ELSE_4)==0){ 
                                condition=1;
                                    if (condition_count>0 && CONDITION[condition_count-1]==0){
                                        CONDITION[condition_count]=0;
                                        condition_etat=0;
                                    } else if (CONDITION[condition_count]==1 ) {
                                        CONDITION[condition_count]=0;
                                        condition_etat=0;
                                    } else if (CONDITION[condition_count]==0) {
                                        CONDITION[condition_count]=1;
                                        condition_etat=1;
                                    }
/*****************************************WHILE********************************/
                            } else if (regex_match_const(tmp,TOKEN_WHILE_1)==0 || regex_match_const(tmp,TOKEN_WHILE_2)==0 || regex_match_const(tmp,TOKEN_WHILE_3)==0 || regex_match_const(tmp,TOKEN_WHILE_4)==0) {
                                //condition = 1 alors condition activé
                                // condition_etat= 1 alors condition vraie
                                condition=1;
                                condition_count++; 
                                CONDITION_TYPE[condition_count]=1;
                                
                                if (condition_count>0 && CONDITION[condition_count-1]==0){
                                    condition_etat=0;
                                    CONDITION_POS[condition_count]=currentpos;
                                    CONDITION[condition_count]=0;
                                } else if (regex_match(tmp,"\\(.*!=.*\\)")==0){ //si condition de type pas égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*!=)"),strlen(regex(tmp,"(\\(.*!=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(!=.*\\))"),strlen(regex(tmp,"(!=.*\\))"))-1,1,""),0,2,"");
                                    if (strcmp(var_name, var_value)==0) {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    } else {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    }
                                } else if (regex_match(tmp,"\\(.*==.*\\)")==0){ //si condition de type égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*==)"),strlen(regex(tmp,"(\\(.*==)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(==.*\\))"),strlen(regex(tmp,"(==.*\\))"))-1,1,""),0,2,"");
                                    if (strcmp(var_name, var_value)==0) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*>=.*\\)")==0){ //si condition de type plus grand ou égal
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*>=)"),strlen(regex(tmp,"(\\(.*>=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(>=.*\\))"),strlen(regex(tmp,"(>=.*\\))"))-1,1,""),0,2,"");
                                    if (str2int(var_name)>=str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*>.*\\)")==0){ //si condition de type plus grand
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*>)"),strlen(regex(tmp,"(\\(.*>)"))-1,1,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(>.*\\))"),strlen(regex(tmp,"(>.*\\))"))-1,1,""),0,1,"");
                                    if (str2int(var_name)>str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                } else if (regex_match(tmp,"\\(.*<=.*\\)")==0){ //si condition de type plus petit ou égal
                                    
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*<=)"),strlen(regex(tmp,"(\\(.*<=)"))-2,2,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(<=.*\\))"),strlen(regex(tmp,"(<=.*\\))"))-1,1,""),0,2,"");
                                    if (str2int(var_name)<=str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                        
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                    
                                } else if (regex_match(tmp,"\\(.*<.*\\)")==0){ //si condition de type plus petit
                                    var_name=str_replace(str_replace(regex(tmp,"(\\(.*<)"),strlen(regex(tmp,"(\\(.*<)"))-1,1,""),0,1,"");
                                    var_value=str_replace(str_replace(regex(tmp,"(<.*\\))"),strlen(regex(tmp,"(<.*\\))"))-1,1,""),0,1,"");
                                    if (str2int(var_name)<str2int(var_value)) {
                                        condition_etat=1;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=1;
                                    } else {
                                        condition_etat=0;
                                        CONDITION_POS[condition_count]=currentpos;
                                        CONDITION[condition_count]=0;
                                    }
                                }
/*************************************FIN CONDITION****************************/
                            } else if (regex_match_const(tmp,TOKEN_IF_ELSE_END_1)==0){
                                //printf("%lu == %lu",afficherEndFonction(mes_endfonctions, tmp_var_name), ftell (SCRIPT));
                                    if (afficherEndFonction(mes_endfonctions, tmp_var_name)== (ftell (SCRIPT)+strlen(tmp))) {
                                        function_count--;
                                        fseek( SCRIPT, FUNCTION[function_count-1], SEEK_SET );
                                    } else if (CONDITION_TYPE[condition_count]==0) { //si c'est une condition IF
                                        condition_count--;
                                        if (condition_count>=0 ) {
                                            if (CONDITION[condition_count]==1){
                                                condition_etat=1;
                                            }else {
                                                condition_etat=0;
                                            }
                                            condition=1;
                                        } else {
                                            condition=0;
                                            condition_etat=0;
                                        }
                                    } else if (CONDITION_TYPE[condition_count]==1) { //si c'est une boucle WHILE
                                        condition_count--;
                                        if ((condition_count+1)>=0 ) {
                                            if (CONDITION[condition_count+1]==1){
                                                condition_etat=1;
                                                fseek( SCRIPT, CONDITION_POS[condition_count+1], SEEK_SET );
                                                condition=1;
                                            } else {
                                                if (CONDITION[condition_count]==1) condition_etat=1;
                                                else condition_etat=0;
                                                if (condition_count+1<=0) condition=0;
                                                else condition=1;
                                            }
                                            
                                        } else {
                                            condition=0;
                                            condition_etat=0;
                                        }
                                    } else if (CONDITION_TYPE[condition_count]==3) { //si c'est une function
                                            condition_count--;
                                            if (condition_count>=0 ) {
                                                if (CONDITION[condition_count]==1){
                                                    condition_etat=1;
                                                }else {
                                                    condition_etat=0;
                                                }
                                                condition=1;
                                            } else {
                                                condition=0;
                                                condition_etat=0;
                                            }
                                            libererEndFonction(mes_endfonctions, tmp_var_name);
                                            tmppos = ftell (SCRIPT)+strlen(tmp);
                                            mes_endfonctions=ajouterEndFonction(mes_endfonctions, tmp_var_name, tmppos);
                                    }
                            } else if (regex_match_const(tmp,TOKEN_FUNCTION_NAME_START)==0){ //function
                                if ((condition==1 && condition_etat > 0) || (condition==0)) {
                                   tmp_var_name=str_strip(tmp);
                                   function_count++;
                                    FUNCTION[function_count]=ftell (SCRIPT);
                                    fseek( SCRIPT, afficherFonction(mes_fonctions, tmp_var_name), SEEK_SET );
                                }
                            } else if (regex_match_const(tmp,TOKEN_INCLUDE_1)==0 || regex_match_const(tmp,TOKEN_INCLUDE_2)==0){ //function include
                                if ((condition==1 && condition_etat > 0) || (condition==0)) {
                                    char *tmp_include = (char *)malloc(sizeof(char) * LG_MAX);
                                    tmp_include=regex(tmp,"\\((.*)\\)");
                                    tmp_include=str_replace(tmp_include,0,1,"");
                                    tmp_include=str_replace(tmp_include,strlen(tmp_include)-1,1,"");
                                    if (pos_html2==0) parseScript(fopen(tmp_include,"r"), 0);
                                    else parseScript(fopen(tmp_include,"r"), 1);
                                    free(tmp_include);
                                }
                            } else if (regex_match_const(tmp,TOKEN_MARKER_SET)==0){ //Ajout d'un marker
                                tmp_tmp=regex(tmp,"^:(.*)");
                                var_name=str_replace(tmp_tmp,0,1,"");
                                libererMarker(mes_markers, var_name);
                                tmppos = ftell (SCRIPT);
                                mes_markers=ajouterMarker(mes_markers, var_name, tmppos);
                                //printf("%s->%d", var_name, afficherMarker(mes_markers, var_name));
                            } else if (regex_match_const(tmp,TOKEN_MARKER_GET)==0){ //aller jusqu'au marker
                                if ((condition==1 && condition_etat > 0) || (condition==0)) {
                                    tmp_tmp=regex(tmp,"^->(.*)");
                                    var_name=str_replace(tmp_tmp,0,2,"");
                                    //printf("[DBG] %s\n<br>",var_name);
                                    condition=0;
                                    condition_etat = 0;
                                    //condition_count=0;
                                    fseek( SCRIPT, afficherMarker(mes_markers, var_name), SEEK_SET );
                                }
                            } else {
                                if (pos_html2<=0) {
                                    printf("Content-type: text/html;charset=utf-8\n\n");
                                    pos_html2=1;
                                }

                                    if (condition==1 && condition_etat > 0) printf("%s\n",tmp);
                                    else if (condition==0) printf("%s\n",tmp);
                            }
                        }
                        
                        //if (multiline==0) memset(tmp, 0, LG_MAX);
                        break;
                    case '$':
                        display_var=1;
                        break;
                    case '[':
                        if (display_var==1) {
                            display_var=2;
                            if (strlen(tmp_tmp)>=0) strcpy(tmp_tmp,"");
                        } else {
                            strncat(tmp, &c, 1);
                        }
                        break;
                    case ']':
                        if (display_var==2) {
                            display_var=0;
                            strcat(tmp, afficherVariable(mes_variables, tmp_tmp));
                            //strcat(tmp, getVar(tmp_tmp,VARIABLES, ENV));
                        } else {
                            strncat(tmp, &c, 1);
                        }
                        break;
                    default:
                        if (display_var==1) {
                            display_var=0;
                            strncat(tmp, "$", 1);
                        }

                        if (display_var==2) {
                            strncat(tmp_tmp, &c, 1);
                            continue;
                        } else if (i==0) {
                            if (c==' ' && multiline==0) { ctmp=1; continue; }
                        } else {
                            if (c==' ' && ctmp==1 && multiline==0) {  continue;}
                            ctmp=0;
                        }
                        strncat(tmp, &c, 1);
                        break;
                }
            }
            
        }
        fclose(SCRIPT);
        free(tmp);
        free(tmp_tmp);
        free(tmp_tmp_tmp);
        free(ligne_courante);
        free(var_name);
        free(tableau_value);
       // memset(c, 0, 0);
        return 0;
    } else {
        return 1;
    }

}


int main(int argc, char *argv[], char *envp[]) {
    char* REQUESTMETHOD;
    char *POST ;
    int fileup=0;
    REQUESTMETHOD=getenv("REQUEST_METHOD");
    char* GET;
    if(/*(str_istr(REQUESTMETHOD,"GET")> -1 ) &&*/ (GET=getenv("QUERY_STRING"))) {
        urldecode(GET);
        getVarsRequest(GET);
    }
    char* COOKIE;
    if((COOKIE=getenv("HTTP_COOKIE"))) {
        urldecode(COOKIE);
        getVarsRequest(COOKIE);
    }
    
    if (str_istr(REQUESTMETHOD,"POST")> -1 ) {
        int content_length;
        if ( strcasecmp(getenv("CONTENT_TYPE"), "application/x-www-form-urlencoded")) {
            if( strcmp(getenv("CONTENT_TYPE"), "multipart/form-data")) {
                //Upload file
                fileup=uploadFile();
            } else {
                printf("Content-type: text/html;charset=utf-8\n\n");
                printf("LeeWee: Unsupported Content-Type.\n") ;
                return 1;
            }
        }
        if (fileup==0) {
            if ( !(content_length = atoi(getenv("CONTENT_LENGTH")))) {
                printf("Content-type: text/html;charset=utf-8\n\n");
                printf("LeeWee: No Content-Length was sent with the POST request.\n") ;
                return 1;
            }
            if ( !(POST= (char *) malloc(content_length+1))) {
                printf("Content-type: text/html;charset=utf-8\n\n");
                printf("LeeWee: Couldn't malloc for POST.\n");
                return 1;
            }
            if (!fread(POST, content_length, 1, stdin)) {
                printf("Content-type: text/html;charset=utf-8\n\n");
                printf("LeeWee: Couldn't read CGI input from STDIN.\n");
                return 1;
            }
            urldecode(POST);
            getVarsRequest(POST);
        }
    }
    
    
    time_t timestamp;
    time(&timestamp);
    
    char *session_name=(char *)malloc (sizeof (char) * LG_MAX);
    sprintf(session_name, "LW_%lu", timestamp);

    if (strcmp(afficherVariable(mes_variables, "LW_SESSION"), "")!=0) {

        if (parseScript(fopen(afficherVariable(mes_variables, "LW_SESSION"),"r"), 0)!=0){
            //fclose(fopen(session_name, "w+"));
            printf("Set-Cookie: LW_SESSION=%s; expires=%u\n", session_name,(unsigned)time(NULL)+900);
            mes_variables=ajouterVariable(mes_variables, "LW_SESSION", session_name);
        } else {
            printf("Set-Cookie: LW_SESSION=%s; expires=%u\n", afficherVariable(mes_variables, "LW_SESSION"),(unsigned)time(NULL)+900);
        }
    } else {
        //fclose(fopen(session_name, "w+"));
        printf("Set-Cookie: LW_SESSION=%s; expires=%u\n", session_name,(unsigned)time(NULL)+900);
        mes_variables=ajouterVariable(mes_variables, "LW_SESSION", session_name);
    }
    
    if (parseScript(fopen(argv[1],"r"), 0)==0){
        libererListeVariables(mes_variables);
        return 0;
    } else {
        printf("LeeWee Script by David Lhoumaud\nGnu/GPL\n\nUsage : lw <FILENAME>\nExemple : lw index.cgi\n");
        return 1;
    }

}
