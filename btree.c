/*
Vinícius Marques Stocco - Professor: Ricardo Campello
*/

#include <stdio.h>
#include <stdlib.h>

#define MAX_CHAVES   4
#define MAX_FILHOS MAX_CHAVES +1
#define MINIMUM_KEYS 2
#define CHAVE_TAMANHO 7
#define FILHO_ESQUERDO(a,i)(a[i])
#define FILHO_DIREITO(a,i)(a[i+1])
#define OK 0
#define OVERFLOW  1
#define CHAVE_EXISTENTE 2
#define CHAVE_INEXISTENTE 3
#define NOVO 1
#define ATUALIZAR 0
#define MAX_CODIGO CHAVE_TAMANHO
#define MAX_TITULO 82
#define MAX_AUTOR  82
#define MAX_ANO 6
#define MAX_VEICULO 82

#define TAMANHO_REGISTRO 256
#define RIGHT_SIBLING 1
#define LEFT_SIBLING 2
#define NO_SIBLING 0

typedef struct registroTipo
{
        char codigo[MAX_CODIGO];
        char titulo[MAX_TITULO];
        char autor[MAX_AUTOR];
        char ano[MAX_ANO];
        char veiculo[MAX_VEICULO];
}reg;


typedef char chaveTipo[CHAVE_TAMANHO];
typedef int offsetTipo;

typedef struct{
           chaveTipo chave;
           offsetTipo offsetRegistro;

}ChaveOffset;

/* nesta implementação, no campo ChaveOffset, foi propositalmente colocado um índice
   a mais para tratar do overflow
*/
typedef struct ArvoreB_Node{
           int numChaves;
           ChaveOffset chaveOffset[MAX_CHAVES+1];
           offsetTipo filhos[MAX_FILHOS+1];
           offsetTipo pageOffset;
}ArvoreB_Node;

/*estrutura auxiliar para o formato de dados no arquivo de índice;
o offset do último filho é lido separadamente */
typedef struct auxStruct{
           offsetTipo proxPagina;
           ChaveOffset registroRef;
}auxStruct;


typedef struct indice_header{
           offsetTipo pagina_raiz;
           offsetTipo topo_pilha;
}indice_header;

typedef  ArvoreB_Node  arvoreB;
/*==============================================================================
                                 Split
================================================================================*/
/*o split não faz a promoção da chave adequada na árvore, esta ação é feita na rotina buscaInsere,
esta rotina somente calcula a chave a ser promovida e faz a divisão entre os nós atual e nó direito do atual */
void split(ArvoreB_Node *Node,ArvoreB_Node *DirNode,ChaveOffset* promo){

     int promoIndex = (MAX_CHAVES+1)/2;
     int i,k=0;

     for(i = promoIndex+1;i<=MAX_CHAVES;++i){
        DirNode->chaveOffset[k] =Node->chaveOffset[i];
        DirNode->filhos[k] =Node->filhos[i];
        k++;
     }
     DirNode->filhos[k] =Node->filhos[MAX_CHAVES+1];

     Node->numChaves = promoIndex;
     DirNode->numChaves = MAX_CHAVES - promoIndex;
     *promo = Node->chaveOffset[promoIndex];


}
/*==============================================================================
                                 readPage
================================================================================*/
//lê o nó com endereço "offset" da árvore no disco e atribui à variável "destNode"
int readPage(FILE *f,offsetTipo offset,ArvoreB_Node* destNode){
   auxStruct buffer[MAX_CHAVES];
   int i;
   fseek(f,offset,SEEK_SET);
   fread(buffer,sizeof(buffer),1,f);
   destNode->numChaves=0;
   for(i=0;i<MAX_CHAVES;++i){
      destNode->filhos[i] = buffer[i].proxPagina;
      destNode->chaveOffset[i]=buffer[i].registroRef;

      if(buffer[i].registroRef.offsetRegistro==-1)
         break;
      else
         destNode->numChaves++;
   }
   fread(&destNode->filhos[MAX_CHAVES],sizeof(offsetTipo),1,f);
   destNode->pageOffset = offset;
   return 0;

}

/*==============================================================================
                                 atualizaHeader
================================================================================*/
inline void atualizaHeader(FILE *f, const indice_header *header){
   fseek(f,0,SEEK_SET);
   fwrite(header,sizeof(indice_header),1,f);
}
/*==============================================================================
                                 RemovePage
================================================================================*/
void removePage(FILE *f,indice_header *header,offsetTipo offsetPage){
   struct aux {
      char delimitador[2];
      offsetTipo proximo;
   } buffer;
   buffer.delimitador[0]='*';
   buffer.delimitador[1]='|';
   buffer.proximo = header->topo_pilha;
   header->topo_pilha = offsetPage;
   fseek (f, header->topo_pilha, SEEK_SET);
   fwrite(&buffer,sizeof(buffer),1,f);
}
/*==============================================================================
                                RemoveRegistro
================================================================================*/
void removeRegistro(FILE *f,offsetTipo offsetRegistro){
   char buffer[2]={'*','|'};
   fseek(f,offsetRegistro,SEEK_SET);
   fwrite(&buffer,sizeof(buffer),1,f);
}
/*==============================================================================
                                 writePage
================================================================================*/
/* rotina para atualizar o arquivo de índice, escrevendo os valores no header e na página correspondente
   Obs: o último argumento se trata de um flag que diz se a operação é de atualização ou de inserção
   de um novo nó
*/
offsetTipo writePage (FILE *f,indice_header *header,const ArvoreB_Node *node,const int flag)
{
     int i;
     offsetTipo novoTopo = header->topo_pilha;
     auxStruct buffer[MAX_CHAVES];
     offsetTipo ultimo_filho=-1;
     offsetTipo writePos;

   for(i=0;i<node->numChaves;++i){
      buffer[i].proxPagina = node->filhos[i];
      buffer[i].registroRef = node->chaveOffset[i];
   printf("\nchave %d: %s",i,buffer[i].registroRef.chave);
   }



   if(node->numChaves<MAX_CHAVES){
      buffer[node->numChaves].proxPagina =node->filhos[node->numChaves];

      for(i=node->numChaves;i<MAX_CHAVES;++i)
         buffer[i].registroRef.offsetRegistro=-1;
   }
   else
      ultimo_filho = node->filhos[node->numChaves];



     if (flag == NOVO){
        if (header->topo_pilha != -1)
        {
           fseek (f, header->topo_pilha + 2*sizeof(char), SEEK_SET);
           fread (&novoTopo, sizeof(offsetTipo), 1, f);
           fseek (f, header->topo_pilha, SEEK_SET);
           writePos = header->topo_pilha;
        }
        else{
           fseek(f,0,SEEK_END);
           writePos = ftell (f);
        }
        printf("\nNovo no na posicao %: %d",writePos);
     }
     else if (flag == ATUALIZAR){
        fseek (f, node->pageOffset, SEEK_SET);
        writePos = node->pageOffset;
        printf("\natualiza no na posicao: %d",node->pageOffset);
     }

     fwrite(buffer, sizeof(auxStruct), MAX_CHAVES, f);
     fwrite(&ultimo_filho, sizeof(offsetTipo), 1, f);
     //printf("\nescreveu %d bytes",ftell(f)-writePos);
     if (header->topo_pilha != -1 && flag == NOVO)
     {
        header->topo_pilha = novoTopo;
        atualizaHeader (f, header);
     }


     return writePos;
}
/*==============================================================================
                                 inicializaNo
================================================================================*/
void inicializaNo(ArvoreB_Node* node){
     int i=0;
     for(i=0;i<MAX_FILHOS;++i){
        node->filhos[i]=-1;
     }
     node->numChaves =0;

}
/*==============================================================================
                                 criarIndice
================================================================================*/
//inicializa o arquivo de índice
int criarIndice(const char *nome_arq,ArvoreB_Node *raiz,indice_header *indiceHeader){
   FILE *indice;
   int i;
   for(i=0;i<=MAX_FILHOS;++i)
      raiz->filhos[i] =-1;

   raiz->pageOffset = indiceHeader->pagina_raiz =sizeof(indice_header);
   raiz->numChaves = 0;

   raiz->filhos[0]=-1;
   indiceHeader->topo_pilha = -1;
   if((indice = fopen(nome_arq,"wb"))== NULL) return -1;
   fwrite(indiceHeader,sizeof(indice_header),1,indice);
   writePage(indice,indiceHeader,raiz,NOVO);
   fclose(indice);
   return 0;

}
inline int criarDados(const char *nome_arq){
   FILE *dadosArq;
   if((dadosArq = fopen(nome_arq,"wb"))== NULL) return -1;
}
/*==============================================================================
                                 chaveCmp
================================================================================*/
#define CHAVES_IGUAIS 0
#define CHAVE1_MAIOR  1
#define CHAVE1_MENOR -1
int chaveCmp(const chaveTipo chave1,const chaveTipo chave2){
   int i=-1;
   while((chave1)[++i]!='\0'&& (chave1)[i]==(chave2)[i]);

   if((chave1)[i]==(chave2)[i])
      return CHAVES_IGUAIS;
   else if((chave1)[i]>(chave2)[i])
      return CHAVE1_MAIOR;
   else
      return CHAVE1_MENOR;

}
/*==============================================================================
                                 buscaBinaria
================================================================================*/
int buscaBinaria(const ChaveOffset regRef[],int n, const chaveTipo chave, int *find)
{

  *find = 0;
   int meio=0, i=0, j=n-1;


  while (i <= j)
  { printf("\ncomparando chave %s, com a chave %s ",regRef[meio].chave,chave);
    meio = (i + j)/2;
    if ( chaveCmp(chave,regRef[meio].chave) == CHAVES_IGUAIS )
    {
       *find = 1;
       return (meio); //Encontrou a chave e retorna sua posíção
    }
    else
    {
        if ( chaveCmp(chave,regRef[meio].chave) == CHAVE1_MENOR ) // queremos a chave 1 menor
        {
            j = meio - 1;
        }
         else i = meio + 1;
    }
  }
  if ( n>0 && chaveCmp(chave,regRef[meio].chave) == CHAVE1_MAIOR )
       meio++;

  return meio; //Não encontrou a chave, retorna a posição do ponteiro para o filho

}
/*==============================================================================
                               isNoFolha
================================================================================*/
inline int isNoFolha(const ArvoreB_Node *node){  return (node->filhos[0]==-1); }

/*==============================================================================
                             inserirChaveNo
================================================================================*/
int inserirChaveNo(ArvoreB_Node *Node,const ChaveOffset *chave,int chavePos)
{

    int aux = Node->numChaves;
    int overflow;

    //busca a posiçao apropriada para inserir a chave
    while ((aux > chavePos) && (aux <=MAX_CHAVES))
    {

        Node->chaveOffset[aux]=Node->chaveOffset[aux-1];
        FILHO_DIREITO(Node->filhos,aux)=FILHO_ESQUERDO(Node->filhos,aux);
        aux--;
    }

    strcpy (Node->chaveOffset[chavePos].chave,chave->chave);
    Node->chaveOffset[chavePos].offsetRegistro = chave->offsetRegistro;


   Node->numChaves++;

   if(Node->numChaves>MAX_CHAVES)
      overflow =1;
   else
      overflow =0;

   return overflow;
}
inline int changeChaveNo(ArvoreB_Node *node,const ChaveOffset *chave,int chavePos){

}
/*==============================================================================
                                underflow
================================================================================*/
inline int underflow(ArvoreB_Node *node){return node->numChaves<MINIMUM_KEYS;}
/*==============================================================================
                             canRedistribute
================================================================================*/

int canRedistribute(FILE *indice,const ArvoreB_Node* fatherNode,ArvoreB_Node *siblingNode,int childKeyPos){


   if(childKeyPos<fatherNode->numChaves){
      readPage(indice,FILHO_DIREITO(fatherNode->filhos,childKeyPos),siblingNode);
      if(siblingNode->numChaves>MINIMUM_KEYS){
         return RIGHT_SIBLING;
      }
   }

   childKeyPos--;
   if(childKeyPos>=0){
      readPage(indice,FILHO_ESQUERDO(fatherNode->filhos,childKeyPos),siblingNode);
      if(siblingNode->numChaves>MINIMUM_KEYS){
         return LEFT_SIBLING;
      }
   }
   return NO_SIBLING;

}

/*==============================================================================
                              removeKey
================================================================================*/
/* rotina auxiliar para resolveUnderflow, realiza a remoção da chave dada no nó "curNode"
*/
int removeKey (ArvoreB_Node *curNode, int pos)
{

    if (pos < curNode->numChaves)
    {
       //shift dos elementos para trás
       while (pos < curNode->numChaves - 1)
       {
             curNode->chaveOffset[pos] = curNode->chaveOffset[pos + 1];
             pos++;
       }
    }
    curNode->numChaves--;
    return (underflow (curNode)); //flag
}

/*==============================================================================
                              redistribute
================================================================================*/
/* rotina auxiliar para resolveUnderflow, realiza a redistribuição das chaves na árvore

Obs1: f = arquivo de índice
Obs2: a rotina somente resolve o underflow, promovendo apenas uma chave do nó "AvailNode", deixando assim
o nó curNode (nó com underflow) com o número mínimo de chaves
*/
/*void redistribute (FILE *f, indice_header *header, ArvoreB_Node *curNode, ArvoreB_Node *father, int posPai)
{
     int i = 0;
     ArvoreB_Node AvailNode;
     ChaveOffset aux1 = father->chaveOffset[posPai];
     printf("\nredistribuindo");
     if (father->filhos[posPai] == curNode->pageOffset) //filho esquerdo
     {
        father->chaveOffset[posPai] = AvailNode->chaveOffset[AvailNode->numChaves - 1];
        curNode->chaveOffset[i + 1] = curNode->chaveOffset[i];
        curNode->chaveOffset[i] = aux1;
        AvailNode->numChaves--;

     }
     else
     {
         if (father->filhos[posPai + 1] == curNode->pageOffset) //filho direito
         {
            father->chaveOffset[posPai] = AvailNode->chaveOffset[i];
            curNode->chaveOffset[i + 1] = aux1;
            removeKey (AvailNode, i);
         }
     }
     curNode->numChaves++;
     writePage (f, header, father, ATUALIZAR);
     writePage (f, header, AvailNode, ATUALIZAR);
     writePage (f, header, curNode, ATUALIZAR);
}*/

int redistribute(FILE *f, indice_header *header, ArvoreB_Node *node, ArvoreB_Node *father,ArvoreB_Node *sibling,int siblingType, int childKeyPos){

   if(siblingType == LEFT_SIBLING){

      inserirChaveNo(node,&father->chaveOffset[childKeyPos],0);
      father->chaveOffset[childKeyPos]= sibling->chaveOffset[sibling->numChaves];
      removeKey(sibling,sibling->numChaves);

   }else if(siblingType  == RIGHT_SIBLING){

      inserirChaveNo(node,&father->chaveOffset[childKeyPos],node->numChaves);
      father->chaveOffset[childKeyPos]= sibling->chaveOffset[0];
      removeKey(sibling,0);

   }
    else
       return 0;

   writePage(f,header,father,ATUALIZAR);
   writePage(f,header,sibling,ATUALIZAR);
   writePage(f,header,node,ATUALIZAR);
   return 1;
}

/*==============================================================================
                              concatena
================================================================================*/
/* rotina auxiliar para resolveUnderflow, realiza a concatenação das chaves na árvore
   o nó curNode é o nó a ser liberado
Obs1: f = arquivo de índice
Obs2: por default a concatenação acontecerá no filho direito
*/
/*int concatena (FILE *f, indice_header *header, ArvoreB_Node *curNode, ArvoreB_Node *father, int posPai)
{
     int flag = OK;
     int i = 0;
     ArvoreB_Node AvailNode;
     int aux = AvailNode.numChaves;

     ChaveOffset aux1 = father->chaveOffset[posPai];

     if(posPai<curNode->numChaves){
        readPage(f,FILHO_DIREITO(curNode->filhos,posPai),&AvailNode);
     }
     else{
        posPai--;
        if(posPai>=0){
           readPage(f,FILHO_ESQUERDO(curNode->filhos,posPai),&AvailNode);
        }
     }

     if (father->filhos[posPai] == curNode->pageOffset) //filho esquerdo
     {
        inserirChaveNo (AvailNode, &aux1, aux);
        inserirChaveNo (AvailNode, &curNode->chaveOffset[i], aux + 1);
        if (AvailNode->numChaves > MAX_CHAVES)
           flag = OVERFLOW;
        removeKey (father, posPai);
        int cont = posPai;
        while (cont < father->numChaves)
        {
              father->filhos[cont] = father->filhos[cont + 1];
              cont++;
        }
     }
     else
     {
         if (father->filhos[posPai + 1] == curNode->pageOffset) //filho direito
         {
            inserirChaveNo (&AvailNode, &aux1, i + 1);
            inserirChaveNo (&AvailNode, curNode->chaveOffset[i], i);
            if (AvailNode->numChaves > MAX_CHAVES)
               flag = OVERFLOW;
            removeKey (father, posPai);
            int cont = posPai - 1;
            while (cont < father->numChaves)
            {
                  father->filhos[cont] = father->filhos[cont + 1];
                  cont++;
            }
         }
     }
     writePage (f, header, father, ATUALIZAR);
     writePage (f, header, &AvailNode, ATUALIZAR);
     header->topo_pilha = curNode->pageOffset;
     //empilhar curNode
     return flag;
}*/

int concatena (FILE *f, indice_header *header, ArvoreB_Node *node, ArvoreB_Node *father,
ArvoreB_Node *sibling,int siblingType, int childKeyPos){
   int i,aux,pos;
   offsetTipo offsetRemovido;
   for(i=0;i<sibling->numChaves;++i){
      pos = buscaBinaria(node->chaveOffset,node->numChaves,sibling->chaveOffset[i].chave,&aux);
      inserirChaveNo(node,&sibling->chaveOffset[i],pos);
   }

   pos = buscaBinaria(node->chaveOffset,node->numChaves,father->chaveOffset[childKeyPos].chave,&aux);
   inserirChaveNo(node,&father->chaveOffset[childKeyPos],pos);
   removeKey(father,childKeyPos);
   offsetRemovido = sibling->pageOffset;
   writePage(f,header,father,ATUALIZAR);
   removePage(f,header,offsetRemovido);
   writePage(f,header,node,ATUALIZAR);
   return offsetRemovido;

};

/*==============================================================================
                              resolveUnderflow
================================================================================*/
/*void resolveUnderflow (FILE *indice, indice_header *header, ArvoreB_Node *node, ArvoreB_Node *father, int childPos)
{
     ChaveOffset chavepro;
     ArvoreB_Node DirNode, sibling;
     int siblingType;
     printf("\ntrantando de underflow.");

     if (isNoFolha(node)&& (siblingType = canRedistribute (indice, father, &sibling, childPos))== NO_SIBLING)
     {  printf("\naplicando redistribuicao.");
        if(siblingType == LEFT_SIBLING)
           childPos--;
        redistribute (indice, header, node, father,&sibling,siblingType,childPos);
     }
     else
     {    printf("\naplicando concatenacao.");
          concatena (indice, header, node, father,&sibling,siblingType,childPos);
          /*{
             readPage (indice, father->filhos[childPos], &DirNode);
             split (&sibling, &DirNode, &chavepro);
             inserirChaveNo (father, &chavepro, childPos);
          }*/
/*     }
}*/

void resolveUnderflow (FILE *indice, indice_header *header, ArvoreB_Node *node, ArvoreB_Node *father, int childPos)
{
     ChaveOffset chavepro;
     ArvoreB_Node DirNode, sibling;
     int siblingType;

     printf("\ntrantando de underflow.");

     if (isNoFolha(node)&& (siblingType = canRedistribute (indice, father, &sibling, childPos))== NO_SIBLING)
     {  printf("\naplicando redistribuicao.");
        if(siblingType == LEFT_SIBLING)
           childPos--;
        redistribute (indice, header, node, father,&sibling,siblingType,childPos);
     }
     else
     {    printf("\naplicando concatenacao.");
          if(childPos<father->numChaves){
             siblingType = LEFT_SIBLING;
             childPos--;
             readPage(indice,FILHO_ESQUERDO(father->filhos,childPos),&sibling);
          }
          else{
             readPage(indice,FILHO_DIREITO(father->filhos,childPos),&sibling);
             siblingType = RIGHT_SIBLING;
          }

          concatena (indice, header, node, father,&sibling,siblingType,childPos);
          /*{
             readPage (indice, father->filhos[childPos], &DirNode);
             split (&sibling, &DirNode, &chavepro);
             inserirChaveNo (father, &chavepro, childPos);
          }*/
     }
}


/*void getSuccessor(FILE *indice,ArvoreB_Node *curNode,int chavePos,ArvoreB_Node *sucNode){
   readPage(indice,FILHO_DIREITO(curNode,chavePos),sucNode);
   while(!isNoFolha(sucNode))
      readPage(indice,FILHO_ESQUERDO(sucNode,0),sucNode);

}*/
/*==============================================================================
                                   auxSwap
================================================================================*/
void auxSwap(FILE *indice,indice_header *header, ArvoreB_Node *node,ArvoreB_Node *father,ChaveOffset *promo){
   int flag;

   if(!isNoFolha(node)){
      ArvoreB_Node sucNode;
      readPage(indice,FILHO_ESQUERDO(node->filhos,0),&sucNode);
      auxSwap(indice,header,&sucNode,node,promo);
      if (underflow(node)){
         resolveUnderflow(indice,header,node,father,0);
      }
   }else{
      *promo = node->chaveOffset[0];
      removeKey(node,0);
      if(underflow(node))
         resolveUnderflow(indice,header,node,father,0);
      else
         writePage(indice,header,node,ATUALIZAR);
   }
}
/*==============================================================================
                                   swapNode
================================================================================*/
int swapNode(FILE *indice,indice_header *header,ArvoreB_Node *curNode,int chavePos){
   ArvoreB_Node sucNode;
   ChaveOffset promo;

   readPage(indice,FILHO_DIREITO(curNode->filhos,chavePos),&sucNode);
   auxSwap(indice,header,&sucNode,curNode,&promo);
   curNode->chaveOffset[chavePos]= promo;
   writePage(indice,header,curNode,ATUALIZAR);
}


/*==============================================================================
                              buscaRemoveAux
================================================================================*/
int buscaRemoveAux(FILE *indice,indice_header *header,ArvoreB_Node *curNode,ArvoreB_Node *father,chaveTipo chave,int fatherIndex){
   int found;
   int chavePos = buscaBinaria(curNode->chaveOffset,curNode->numChaves,chave,&found);
   printf("\nposicao de chave em no interno = %d",chavePos);
   if(found){
      printf("\nACHOU !!!!!");
      if(isNoFolha(curNode)){
         printf("\nremovendo chave %s ",chave);
         removeKey(curNode,chavePos);
         printf("\nagora o no tem %d chaves e o minimo eh %d",curNode->numChaves,MINIMUM_KEYS);
         printf("\nvalor de underflow = %d ",underflow(curNode));
         if(underflow(curNode)){
            resolveUnderflow(indice,header,curNode,father,fatherIndex);
         }
         else
            writePage(indice,header,curNode,ATUALIZAR);

      }
      else{
         printf("\naplicando swap.");
         swapNode(indice,header,curNode,chavePos);
         if(underflow(curNode)){
            resolveUnderflow(indice,header,curNode,father,fatherIndex);
         }
      }
      return CHAVE_EXISTENTE;
   }
   else if(!isNoFolha(curNode)){
      ArvoreB_Node noFilho;
      int flag;

      readPage(indice,curNode->filhos[chavePos],&noFilho);
      flag = buscaRemoveAux(indice,header,&noFilho,curNode,chave,chavePos);
      if(underflow(curNode)){
         resolveUnderflow(indice,header,curNode,father,fatherIndex);
      }

      return flag;
   }
   return CHAVE_INEXISTENTE;
}
/*==============================================================================
                              buscaRemove
================================================================================*/
int buscaRemove(FILE *indice,indice_header *header,ArvoreB_Node *raiz,chaveTipo chave){
   ArvoreB_Node noFilho;
   int found;
   int flag;
   int chavePos = buscaBinaria(raiz->chaveOffset,raiz->numChaves,chave,&found);
   if(found){
      printf("\naplicando swap na raiz.");
      swapNode(indice,header,raiz,chavePos);
      flag = CHAVE_EXISTENTE;
   }
   else{
      readPage(indice,raiz->filhos[chavePos],&noFilho);
      flag = buscaRemoveAux(indice,header,&noFilho,raiz,chave,chavePos);
   }

   if(raiz->numChaves == 0){
      printf("\na arvore diminuiu a sua altura.");
      raiz->pageOffset = header->pagina_raiz = FILHO_ESQUERDO(raiz->filhos,0);
      writePage(indice,header,raiz,ATUALIZAR);
      atualizaHeader(indice,header);
      //readPage(indice,raiz->pageOffset,raiz);
   }
   printf("\nchave 0 da raiz = %s",raiz->chaveOffset[0].chave);
   return flag;
}
/*==============================================================================
                              buscaInsere
================================================================================*/
/* realiza a busca da posição com a busca binária e insere no nó adequado, utilizando as rotinas auxiliares e
  tratando de overflow, além de escrever no arquivo de índice
*/
int buscaInsere(FILE *indice,indice_header *header,ArvoreB_Node *curNode,ChaveOffset *chave,
                                     offsetTipo *dirOffset,ChaveOffset *promo){


   int exist,overflow=0,flag=OK;

   int chavePos = buscaBinaria(curNode->chaveOffset,curNode->numChaves,chave->chave,&exist);

   if(exist)
      return CHAVE_EXISTENTE;

   if(isNoFolha(curNode)){
      int overflow = inserirChaveNo(curNode,chave,chavePos);
      if(overflow){
         ArvoreB_Node DirNode;
         inicializaNo(&DirNode);
         //printf("\noverflow na folha na posicao: %d",chavePos);
         split(curNode,&DirNode,promo);
         *dirOffset = writePage(indice,header,&DirNode,NOVO);
         flag = OVERFLOW;

      }
      else
         flag = OK;
       printf("\nchave inserida no indice: %d",chavePos);
      writePage(indice,header,curNode,ATUALIZAR);

      return flag;
   }else{
      ArvoreB_Node noFilho;
      inicializaNo(&noFilho);
      //printf("\ndesce na arvore.");
      readPage(indice,curNode->filhos[chavePos],&noFilho);
      //printf("\nchave do filho: %s",noFilho.chaveOffset[0].chave);
      //printf("\nposicao do filho: %d",pos);
      flag = buscaInsere(indice,header,&noFilho,chave,dirOffset,promo);

   }

   if(flag == OVERFLOW){
      overflow = inserirChaveNo(curNode,promo,chavePos);
      FILHO_DIREITO(curNode->filhos,chavePos)=*dirOffset;
      //printf("\nrecebendo overflow propagado.");
      if(overflow){
         ArvoreB_Node DirNode;
         //printf("\noverflow em no interno.");
         inicializaNo(&DirNode);
         split(curNode,&DirNode,promo);
         *dirOffset = writePage(indice,header,&DirNode,NOVO);
      }
      else
         flag = OK;
      writePage(indice,header,curNode,ATUALIZAR);

   }
   return flag;


}
/*==============================================================================
                              buscaChave
================================================================================*/

int buscaChave(FILE *indice, ArvoreB_Node* raiz,chaveTipo chave,offsetTipo *rrn){
   int found,chavePos;
   ArvoreB_Node curNode=*raiz;
   *rrn =-1;

   while(1){

      chavePos = buscaBinaria(curNode.chaveOffset,curNode.numChaves,chave,&found);
      if(found){
         *rrn = curNode.chaveOffset[chavePos].offsetRegistro;
         return CHAVE_EXISTENTE;
      }

      if(!isNoFolha(&curNode))
         readPage(indice,curNode.filhos[chavePos],&curNode);
      else
         return CHAVE_INEXISTENTE;
   }

}

/*==============================================================================
                              LimpaNo
================================================================================*/
inline void LimpaNo(ArvoreB_Node *node){
   node->numChaves =0;
}
/*==============================================================================
                          inserirChaveIndice
================================================================================*/
int inserirChaveIndice(FILE *indice,indice_header *header,ArvoreB_Node *raiz,ChaveOffset *chave){
   offsetTipo dirOffset;
   ChaveOffset promo;
   int flag;
   int i;

   flag = buscaInsere(indice,header,raiz,chave,&dirOffset,&promo);

   if(flag == OVERFLOW){
      LimpaNo(raiz);
      inserirChaveNo(raiz,&promo,0);
      FILHO_DIREITO(raiz->filhos,0)=dirOffset;
      FILHO_ESQUERDO(raiz->filhos,0)= raiz->pageOffset;
      printf("\ndiroffset = %d",dirOffset);
      printf("\nnova Raiz:");
      header->pagina_raiz = raiz->pageOffset = writePage(indice,header,raiz,NOVO);
      atualizaHeader(indice,header);
      return OK;
   }

   return flag;

}
/*==============================================================================
                              inserirRegistro
================================================================================*/
/* rotina que insere o registro no arquivo de dados
Obs1: campos codigo e ano sao fixos, porem sao subtraidos de cont
Obs2: f é o arquivo de dados
*/
int inserirRegistro (FILE *f, reg *novo)
{

     offsetTipo writePos =ftell(f);
     char buffer[TAMANHO_REGISTRO],*p =buffer;
     int i,n;

     fseek(f,0,SEEK_END);

     n = strlen(novo->codigo);
     strncpy(p,novo->codigo,n);
     p+=n;
     *p ='@';
     p++;
     n = strlen(novo->titulo);
     strncpy(p,novo->titulo,n);
     p+=n;
     *p ='@';
     p++;
     n = strlen(novo->autor);
     strncpy(p,novo->autor,n);
     p+=n;
     *p ='@';
     p++;
     n = strlen(novo->ano);
     strncpy(p,novo->ano,n);
     p+=n;
     *p ='@';
     p++;
     n = strlen(novo->veiculo);
     strncpy(p,novo->veiculo,n);
     p+=n;
     *p ='@';

     n = &buffer[255] - p;
     p++;
     for(i=0;i<n;++i)
        p[i]='#';

     fwrite(buffer,sizeof(buffer),1,f);

     return writePos;
}

/*==============================================================================
                              readDataFile
================================================================================*/
void readDataFile(FILE *dataFile,offsetTipo rrn,struct registroTipo *destRegistro){
   char buffer[TAMANHO_REGISTRO];
   int bufTamanho = sizeof(buffer);
   int i=0,k=0;
   char* s = buffer;

   fseek(dataFile,rrn,SEEK_SET);
   fread(buffer,sizeof(buffer),1,dataFile);

   while(i<bufTamanho && s[i]!='@'){
      destRegistro->codigo[i]=s[i];
      ++i;
   }

   if(i<bufTamanho&& s[i]=='@')
      destRegistro->codigo[i]='\0';
   s+=i+1;
   i=0;
   while(i<bufTamanho && s[i]!='@'){
      destRegistro->titulo[i]=s[i];
      ++i;
   }

   if(i<bufTamanho&& s[i]=='@')
      destRegistro->titulo[i]='\0';

   s+=i+1;
   i=0;
   while(i<bufTamanho && s[i]!='@'){
      destRegistro->autor[i]=s[i];
      ++i;
   }

   if(i<bufTamanho&& s[i]=='@')
      destRegistro->autor[i]='\0';

   s+=i+1;
   i=0;
   while(i<bufTamanho && s[i]!='@'){
      destRegistro->ano[i]=s[i];
      ++i;
   }

    if(i<bufTamanho&& s[i]=='@')
      destRegistro->ano[i]='\0';

   s+=i+1;
   i=0;
    while(i<bufTamanho && s[i]!='@'){
      destRegistro->veiculo[i]=s[i];
      ++i;
   }

   if(i<bufTamanho&& s[i]=='@')
      destRegistro->veiculo[i]='\0';

}
/*==============================================================================
                            recuperaRegistro
================================================================================*/

int recuperaRegistro(FILE *dataFile,FILE *indexFile,ArvoreB_Node *raiz,chaveTipo chave,struct registroTipo *destRegistro){
   int rrn;

   buscaChave(indexFile,raiz,chave,&rrn);
   if(rrn!=-1){
      readDataFile(dataFile,rrn,destRegistro);
      return CHAVE_EXISTENTE;
   }else
      return CHAVE_INEXISTENTE;
}
/*==============================================================================
                              readString
================================================================================*/
void readString(char* s){
   int i = -1;
   do{
      i++;
      scanf("%c",&s[i]);
   }while(s[i]!='\n');
   s[i]='\0';
}
/*==============================================================================
                              menuInserir
================================================================================*/
void menuInserir(FILE *dadosArq,FILE *indiceArq,indice_header *header,ArvoreB_Node *raiz){


   reg novoRegistro;
   ChaveOffset regRef;

   printf("\n Entre com a chave: ");
   readString(novoRegistro.codigo);
   printf("\n Entre com o titulo: ");
   readString(novoRegistro.titulo);
   printf("\n Entre com o autor: ");
   readString(novoRegistro.autor);
   printf("\n Entre com o ano: ");
   readString(novoRegistro.ano);
   printf("\n Entre com o veiculo: ");
   readString(novoRegistro.veiculo);
   regRef.offsetRegistro = inserirRegistro(dadosArq,&novoRegistro);
   strncpy(regRef.chave,novoRegistro.codigo,CHAVE_TAMANHO);
   if(inserirChaveIndice(indiceArq,header,raiz,&regRef)== CHAVE_EXISTENTE){
      printf("\n Erro: chave ja existe.");
   }
}
/*==============================================================================
                             menuRemover
================================================================================*/
void menuRemover(FILE *dataFile,FILE *indexFile,indice_header *header,ArvoreB_Node* raiz){
   char buf[100];
   chaveTipo chave;

   printf("\nEntre com a chave:");
   readString(buf);
   strncpy(chave,buf,CHAVE_TAMANHO);
   if(buscaRemove(indexFile,header,raiz,chave)== CHAVE_INEXISTENTE)
      printf("\nErro: chave inexistente.");
   else{
      printf("\nChave removida com sucesso." );
   }
}

void menuLoadFile(){

}
/*==============================================================================
                             menuBuscar
================================================================================*/
void menuBuscar(FILE *dataFile,FILE *indexFile,ArvoreB_Node *raiz){
   char buf[100];
   chaveTipo chave;
   struct registroTipo destRegistro;

   printf("\nEntre com a chave:");
   readString(buf);
   strncpy(chave,buf,CHAVE_TAMANHO);
   if (recuperaRegistro(dataFile,indexFile,raiz,chave,&destRegistro)== CHAVE_EXISTENTE){
      printf("\n Titulo: %s\n Autor: %s\n Ano: %s\n Veiculo: %s\n",destRegistro.titulo,
      destRegistro.autor,destRegistro.ano,destRegistro.veiculo);
   }else
      printf("\n Erro: chave nao encontrada.");
}


/******************************************************************************/
int main(int argc, char *argv[])
{
   ArvoreB_Node raiz;
   indice_header indiceHeader;
   FILE *indexFile,*dataFile;
   int option=0,validOption;

   criarIndice("index.txt",&raiz,&indiceHeader);
   criarDados("data.txt");
   indexFile = fopen("index.txt","r+b");
   fseek(indexFile,sizeof(indice_header),SEEK_SET);
   dataFile = fopen("data.txt","r+b");

   while(option!=4){
      printf("\n Digite o numero da opcao desejada:\n\n");
      printf(" 1. Inserir novo registro\n");
      printf(" 2. Remover registro\n");
      printf(" 3. Buscar registro\n");
      //printf(" 4. Modo de depuracao\n");
      printf(" 4. Encerrar o programa\n\n");

      scanf("%d",&option);
      fflush(stdin);
      validOption = 0;
      switch(option){
         case 1:
              menuInserir(dataFile,indexFile,&indiceHeader,&raiz);
              validOption = 1;
              break;
         case 2:
              menuRemover(dataFile,indexFile,&indiceHeader,&raiz);
              validOption = 1;
              break;
         case 3:
              menuBuscar(dataFile,indexFile,&raiz);
              validOption = 1;
              break;
         case 4:
              break;
         default:
              break;
      }
      if(validOption){
         printf("\n\n Pressione qualquer tecla para continuar...\n");
         getch();
      }
   }

   fclose(indexFile);
   fclose(dataFile);
   return 0;
}

