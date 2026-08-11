#define FUSE_USE_VERSION 31
 
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/*
    citeste toate numele la toti directorii din procs
    luand fiecare director in parte, intra in directorul curent si citeste datele din fiesierul
         stats(nu stiu cum se cheama). Extrage pidul dintr-insul
    Creaza structura arborescenta
    Pe urma creaza fisierele cu structura arborescenta in filesistem


*/



int is_number(char *s)
{
        if(s[0]=='\0')return 0;
        for(int i=0;s[i]!='\0';i++)
        {
                if(!isdigit(s[i]))return 0;
        }
        return 1;
}




typedef struct nod_proces{
        int n;
        char *continut;
        struct nod_proces* urmator;

} nod_proces;


typedef struct graf{
        int nr_noduri;
        nod_proces** adiacenta; 
} graf;
graf* ggraf;


graf* creare_graf(int nr_noduri){
        graf* graf=malloc(sizeof(struct graf));
        graf->nr_noduri=nr_noduri;
        graf->adiacenta=malloc(nr_noduri*sizeof(nod_proces*));
        for(int i=0;i<graf->nr_noduri;i++){
                graf->adiacenta[i]=NULL;
        }
        return graf;
}


nod_proces * creare_nod(int v, char* continut){
        nod_proces* nod_nou=malloc(sizeof(nod_proces));
        nod_nou->urmator=NULL;
        nod_nou->n=v;
        char * cont= malloc(strlen(continut)+1);
        strcpy(cont,continut);
        nod_nou->continut=cont;

        return nod_nou;
}

void adauga_muchie(graf* graf, int sursa, int dest, char* continut){
        nod_proces *copie=graf->adiacenta[sursa];
        // nod_proces *ultim=NULL;
        while(copie!=NULL){
                if(copie->n==dest)return;
                // ultim=copie;
                copie=copie->urmator;
        }
        nod_proces *nod_nou=creare_nod(dest, continut);
        nod_nou->urmator=graf->adiacenta[sursa];
        graf->adiacenta[sursa]=nod_nou;

}

void eliberare_resurse_nod(nod_proces* nod_nou){
        if(nod_nou->urmator!=NULL)eliberare_resurse_nod(nod_nou->urmator);
        free(nod_nou->continut);
        free(nod_nou);
}

void eliberare_resurse_graf(graf *graf){
        for(int i=0;i<graf->nr_noduri;i++){
                if(graf->adiacenta[i]!=NULL)eliberare_resurse_nod(graf->adiacenta[i]);
        }
        free(graf->adiacenta);
        free(graf);
}



void bfs(graf* graf){
        int vizitat[30000]={0};
        int coada[3000];
        int st=0, dr=0;
        vizitat[1]=1;
        coada[dr++]=1;

        // aici adaugam folder pentru procesul 1
        while(st<dr){
                int curent=coada[st++];

                nod_proces* t=graf->adiacenta[curent];
                while(t!=NULL){
                        if(!vizitat[t->n]){
                                vizitat[t->n]=1;
                                coada[dr++]=t->n;

                                //aici adaugam folder pentru celelalte procese
                        }
                        t=t->urmator;
                }
        }

}



static int _getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
        memset(stbuf, 0, sizeof(struct stat));
        char *paths = strdup(path);
        char *p = strtok(paths, "/");
        char *urmatorul=NULL;
        int nod_pornire=0;
        int rezultat=0;
        if(strcmp(path, "/")==0){
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                free(paths);
                return 0;
        }     
        if(strcmp(path, "/status")==0){
                free(paths);
                return -ENOENT;
        }
        while(p!=NULL){
                urmatorul=strtok(NULL, "/");
                if(urmatorul!=NULL){

                        if(is_number(p)){
                                int gasit=0;
                                nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                while(cautare !=NULL){
                                        if(cautare->n==atoi(p)){gasit=1;break;}
                                        cautare=cautare->urmator;
                                }
                                if(gasit){nod_pornire=atoi(p);}
                                else {rezultat= -ENOENT;break;}
                        }
                        else {rezultat= -ENOENT;break;}

                }
                else{
                        if(is_number(p)){
                                int gasit=0;
                                nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                while(cautare !=NULL){
                                        if(cautare->n==atoi(p)){gasit=1;break;}
                                        cautare=cautare->urmator;
                                }
                                if(gasit){
                                        stbuf->st_mode= S_IFDIR | 0755;
                                        stbuf->st_nlink=2;
                                }
                                else {rezultat= -ENOENT;break;}
                        }
                        else if(strcmp(p, "status")==0){
                                stbuf->st_mode = S_IFREG | 0444;
                                stbuf->st_nlink=1;
                                stbuf->st_size=4500;
                        }
                        
                        else {rezultat= -ENOENT;break;}

                }

                p=urmatorul;
        }
        free(paths);
        return rezultat;

}


static int _readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
        filler(buf, ".", NULL, 0, 0);
        filler(buf, "..", NULL, 0, 0);

        if(strcmp(path, "/")==0){
                nod_proces* current=ggraf->adiacenta[0];
                while(current!=NULL){
                        char nume_nod[30];
                        sprintf(nume_nod, "%d", current->n);
                        filler(buf, nume_nod, NULL, 0, 0);
                        current=current->urmator;
                }

        }
        else{
                int NOD;


                char *paths = strdup(path);
                char *p = strtok(paths, "/");
                char *urmatorul=NULL;
                int nod_pornire=0;
                
                while(p!=NULL){
                        urmatorul=strtok(NULL, "/");
                        if(urmatorul!=NULL){

                                if(is_number(p)){
                                        int gasit=0;
                                        nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                        while(cautare !=NULL){
                                                if(cautare->n==atoi(p)){gasit=1;break;}
                                                cautare=cautare->urmator;
                                        }
                                        if(gasit){nod_pornire=atoi(p);}
                                        else {free(paths); return -ENOENT;}
                                }
                                else {free(paths); return -ENOENT;}

                        }
                        else{
                                if(is_number(p)){
                                        int gasit=0;
                                        nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                        while(cautare !=NULL){
                                                if(cautare->n==atoi(p)){gasit=1;break;}
                                                cautare=cautare->urmator;
                                        }
                                        if(gasit){
                                                NOD=atoi(p);
                                        }
                                        else {free(paths); return -ENOENT;}
                                }
                                else if(strcmp(p, "status")==0){
                                        free(paths);
                                        return -ENOTDIR;
                                }

                                else {free(paths); return -ENOENT;}

                        }

                        p=urmatorul;
                }
                free(paths);
                filler(buf, "status", NULL, 0, 0);

                nod_proces *v= ggraf->adiacenta[NOD];
                while(v!=NULL){
                        char nume_nod[30];
                        sprintf(nume_nod,"%d", v->n);
                        filler(buf, nume_nod, NULL, 0, 0);
                        v=v->urmator;
                }

        }

        return 0;
}


static int _read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
        char *paths = strdup(path);
        char *p = strtok(paths, "/");
        char *urmatorul=NULL;
        int nod_pornire=0;
        nod_proces* penultim=NULL;
        while(p!=NULL){
                urmatorul=strtok(NULL, "/");
                if(urmatorul!=NULL){
                
                        if(is_number(p)){
                                int gasit=0;
                                nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                while(cautare !=NULL){
                                        if(cautare->n==atoi(p)){gasit=1;penultim=cautare;break;}

                                        cautare=cautare->urmator;
                                }
                                if(gasit){nod_pornire=atoi(p);}
                                else {free(paths); return -ENOENT;}
                        }
                        else {free(paths); return -ENOENT;}
                
                }
                else{
                        
                        if(strcmp(p, "status")==0){
                                if(penultim==NULL)return -ENOENT;

                                char* copie_continut=penultim->continut;
                                int lungime=strlen(copie_continut);
                                if(offset<lungime){
                                        if(offset+size>lungime){
                                                size=lungime-offset;
                                        }
                                        memcpy(buf, copie_continut+offset, size);
                                        
                                        free(paths);
                                        return size;
                                }
                                else{
                                        free(paths);
                                        return 0;
                                }
                        }
                        else {free(paths); return -ENOENT;}
                
                }
        
                p=urmatorul;
        }
        free(paths);
        return 0;
}


static int _open(const char *path, struct fuse_file_info *fi)
{
        char *paths = strdup(path);
        char *p = strtok(paths, "/");
        char *urmatorul=NULL;
        int nod_pornire=0;
        nod_proces* penultim=NULL;
        while(p!=NULL){
                urmatorul=strtok(NULL, "/");
                if(urmatorul!=NULL){
                
                        if(is_number(p)){
                                int gasit=0;
                                nod_proces* cautare=ggraf->adiacenta[nod_pornire];
                                while(cautare !=NULL){
                                        if(cautare->n==atoi(p)){gasit=1;penultim=cautare;break;}

                                        cautare=cautare->urmator;
                                }
                                if(gasit){nod_pornire=atoi(p);}
                                else {free(paths); return -ENOENT;}
                        }
                        else {free(paths); return -ENOENT;}
                
                }
                else{
                        
                        if(strcmp(p, "status")==0){
                                
                        }
                        else {free(paths); return -ENOENT;}
                
                }
        
                p=urmatorul;
        }
        if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
        return 0;
}





static const struct fuse_operations operatii = {
        .getattr        = _getattr,
        .readdir        = _readdir,
        .open           = _open,
        .read           = _read,
};



int main(int argc, char* argv[]){
    DIR* fd;
    struct dirent* fisier;
    
    ggraf=creare_graf(30000);


    fd=opendir("/proc");
    if (fd == NULL) {
        perror("Eroare");
        return -1;
    }
    fisier=readdir(fd);
    while(fisier){
        struct stat buffer;
        int status;
        
        char path_intreg[1500];

        if (strcmp(fisier->d_name, ".")==0 || strcmp(fisier->d_name, "..")==0){
            fisier=readdir(fd);
            continue;
        }

        snprintf(path_intreg, sizeof(path_intreg), "/proc/%s", fisier->d_name);
        status=stat(path_intreg, &buffer);
        if (status == -1){
            printf("Eroare la cit");
            return -1;
        }
        

        if( (buffer.st_mode & S_IFMT) == S_IFDIR && is_number(fisier->d_name))
        {
                FILE * fptr;
                char path_status[3600]={'\0'};
                char continut_fisier[4500];
                
                snprintf(path_status, sizeof(path_status), "%s/status", path_intreg);   
                fptr = fopen(path_status, "r");
                if(fptr){
                        ssize_t lungime_fisier = fread(continut_fisier, 1, sizeof(continut_fisier), fptr);
                        if(lungime_fisier==0){
                            printf("Eroare");
                        }
                        continut_fisier[lungime_fisier]='\0';
                        fclose(fptr);
                        // printf("%s", continut_fisier);
                        // break;
                        char *subsir=strstr(continut_fisier, "PPid:\t");
                        if(subsir!=NULL){
                                char ch_numar[10]={'\0'};
                                int index=0;

                                // printf("%s",subsir);
                                for(int i=0;subsir[i]!='\n';i++){
                                    if(isdigit(subsir[i])){
                                        ch_numar[index++]=subsir[i];
                                    }
                                }
                                //continut_fisier;
                                int copil = atoi(fisier->d_name);
                                int PPid = atoi(ch_numar);

                                adauga_muchie(ggraf, PPid, copil, continut_fisier);

                                printf("%d\n", PPid);
                        }
                }
            // printf("S-a citit din fisier.");
        }
        fisier=readdir(fd);
    }
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int r=fuse_main(args.argc, args.argv, &operatii, NULL);
    
    eliberare_resurse_graf(ggraf);
    return r;
}

