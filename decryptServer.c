//----------------------------------------------------------------------------

#include "crsUtils.h"

#define PASSWORD_LENGTH 5
#define FIRST_CHAR 33
#define LAST_CHAR 126
#define CHAR_RANGE (1+LAST_CHAR-FIRST_CHAR)
#define RESTART_SYSCALL(result,syscall) \
do { (result)=(syscall); } while(((result)<0)&&(errno==EINTR));

char encrypted[14]; // DES algorithm: 2 chars (salt) + 11 chars + '\0'
long long combinations;
long long sliceSize;
long long sliceCount;
double startTime;

pthread_mutex_t mtx; // synchronise access to the following variables
volatile long long testedCount;
volatile unsigned char *slices;

//
// Cette fonction est juste illustrative ; elle est appelée lorsque le
// programme est invoqué sans argument.
// Elle teste une tranche de l'espace des mots de passe à tester afin
// d'estimer le temps nécessaire pour une recherche exhaustive sur tout cet
// espace.
//
void
estimateExhaustiveSearchDuration(void)
{
//---- pick a slice at random (for testing purpose) ----
long long slice=rand()%sliceCount;
long long start=slice*sliceSize;
long long end=start+sliceSize<=combinations ? start+sliceSize : combinations;

//---- test the combinations of this slice ----
printf("testing only %lld passwords ",end-start);
fflush(stdout);
startTime=getTime();
for(long long attempt=start;attempt<end;++attempt)
  {
  if(!((attempt-start)%((end-start)/10))) { printf("."); fflush(stdout); }
  //---- generate a password from ``attempt'' ----
  long long value=attempt;
  char password[PASSWORD_LENGTH+1];
  for(int i=0;i<PASSWORD_LENGTH;++i)
    {
    password[i]=(char)(FIRST_CHAR+value%CHAR_RANGE);
    value/=CHAR_RANGE;
    }
  password[PASSWORD_LENGTH]='\0';
  //---- test it against the encrypted version ----
  if(!strcmp(encrypted,crypt(password,encrypted)))
    {
    printf("SUCCESS: %s matches %s\n",
           password,encrypted);
    }
  }
double seconds=getTime()-startTime;
printf(" in %g seconds\n",seconds);

//---- extrapolate the duration for an exhaustive search ----
seconds*=combinations/(double)(end-start);
int hours=(int)(seconds/3600.0);
seconds-=3600.0*hours;
int minutes=(int)(seconds/60.0);
seconds-=60.0*minutes;
printf("%d h, %d m and %d s would have been necessary "
       "for an exhaustive search!\n",
       hours,minutes,(int)seconds);
}

//
// Cette fonction doit être exécutée dans un nouveau thread à chaque fois
// qu'un nouveau client se connecte à notre serveur TCP.
//
void *
dialogThread(void *arg)
{
pthread_detach(pthread_self()); // no pthread_join() needed
//---- obtain dialog socket from arg ----
int dialogSocket=*(int*)arg;
(void)dialogSocket; // avoid ``unused variable'' warning
free(arg);

for(;;)
  {
  //---- choose next slice ----
  long long slice=-1;
  pthread_mutex_lock(&mtx);
  for(long long i=0;i<sliceCount;++i)
    {
    if(!slices[i]) // not tested yet
      {
      slice=i;         // test this slice
      slices[slice]=1; // mark it as tested
      break;
      }
    }
  pthread_mutex_unlock(&mtx);

  //---- handle last slices ----
  if(slice==-1) // no more untested slice
    {
    if(testedCount!=sliceCount) // some slices are pending
      {
      continue; // some running clients may fail --> retry just in case
      }
    else // all of them gave a result --> stop
      {
      printf("no more slice\n");
      break;
      }
    }

  //---- send slice to be tested ----
  long long start=slice*sliceSize;
  long long end=start+sliceSize<=combinations ? start+sliceSize : combinations;
  int sendResult=-1;
  char buffer[0x100];
  (void)end; // avoid ``unused variable'' warning
  (void)buffer; // avoid ``unused variable'' warning
  //

  sprintf(buffer,"%s %lld %lld\n", encrypted, start, end);
  sendResult = send(dialogSocket,buffer,strlen(buffer),0);
  //
  // Rédiger une ligne de texte
  //   "mot_de_passe_chiffré indice_de_début indice_de_fin\n"
  // dans ``buffer'' et l'envoyer au client afin qu'il réalise le test de
  // cette tranche.
  //

  //---- mark this slice as untested in case of send failure ----
  if(sendResult==-1)
    {
    perror("send error");
    pthread_mutex_lock(&mtx);
    slices[slice]=0; // this slice will be retried later by another client
    pthread_mutex_unlock(&mtx);
    break;
    }

  //---- receive a reply ----
  int recvResult=recv(dialogSocket,buffer,sizeof(buffer),0);
  printf("Message recu : %s\n",buffer);
  //
  // Obtenir dans ``buffer'' une ligne de texte depuis le client.
  //

  //---- mark this slice as untested in case of receive failure ----
  if(recvResult<=0) {
    perror("recv");
    pthread_mutex_lock(&mtx);
    slices[slice]=0; // this slice will be retried later by another client
    pthread_mutex_unlock(&mtx);
    break;
    }
      buffer[recvResult]='\0';

      //---- analyse reply for this slice ----
      //
      if(strstr(buffer, "SUCCESS");){
	double seconds=getTime()-startTime;
	printf(" in %g seconds\n",seconds);
	//---- extrapolate the duration for an exhaustive search ----
	seconds*=combinations/(double)(end-start);
	int hours=(int)(seconds/3600.0);
	seconds-=3600.0*hours;
	int minutes=(int)(seconds/60.0);
	seconds-=60.0*minutes;
	printf("%d h, %d m and %d s would have been necessary "
	      "for an exhaustive search!\n",
	      hours,minutes,(int)seconds);
    }
  //
  // Si la réponse commence par "SUCCESS" alors le mot suivant correspond
  // au mot de passe découvert ; il suffit alors de l'afficher ainsi que la
  // durée totale de la recherche avant de mettre fin au programme.
  //

  //---- count tested slices and show stats ----
  pthread_mutex_lock(&mtx);
  ++testedCount;
  double done=testedCount/(double)sliceCount;
  double duration=getTime()-startTime;
  printf("%g%% done in %g s (%g s estimated)\n",
         100.0*done,duration,duration/done);
  pthread_mutex_unlock(&mtx);
  }

//---- close dialog socket ----
printf("client disconnected\n");
//
// ... À COMPLÉTER ...
//
// fermer la socket de dialogue
//

return (void *)0;
}




int main(int argc, char ** argv)
{
//---- initialise pseudo-random generator ----
srand(time(NULL)^getpid());

//---- draw a secret password ----
char secret[PASSWORD_LENGTH+1];
for(int i=0;i<PASSWORD_LENGTH;++i)
  {
  secret[i]=(char)(FIRST_CHAR+rand()%CHAR_RANGE);
  }
secret[PASSWORD_LENGTH]='\0';
printf("secret password: %s\n",secret);

//---- produce the encrypted password ----
strcpy(encrypted,crypt(secret,"S7")); // use "S7" as salt for DES algorithm
printf("encrypted password: %s\n",encrypted);

//---- determine the total number of combinations ----
combinations=1;
for(int i=0;i<PASSWORD_LENGTH;++i)
  {
  combinations*=CHAR_RANGE;
  }
printf("there are %lld possible passwords of length %d\n",
       combinations,PASSWORD_LENGTH);

//---- divide these combinations in slices ----
sliceSize=1000*1000;
sliceCount=(combinations+(sliceSize-1))/sliceSize;
slices=(unsigned char *)calloc(sliceCount,sizeof(unsigned char));

//---- initialise startTime for the first thread ----
startTime=-1.0;

//---- check command line arguments ----
if(argc==1)
  { estimateExhaustiveSearchDuration(); exit(0); }
if(argc!=2)
  { fprintf(stderr,"usage: %s port\n",argv[0]); exit(1); }

//---- extract local port number ----
int portNumber;
if(sscanf(argv[1],"%d",&portNumber)!=1)
  { fprintf(stderr,"invalid port %s\n",argv[1]); exit(1); }

//---- create listen socket ----
//


int listenFd,fd,r,on;
struct sockaddr_in addr;

RESTART_SYSCALL(listenFd,socket(PF_INET,SOCK_STREAM,0));
if(listenFd==-1) { perror("socket"); return EXIT_FAILURE; }
on=1; /* autoriser l'option SO_REUSEADDR */
RESTART_SYSCALL(r,setsockopt(listenFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int)));
if(r==-1) { perror("setsockopt SO_REUSEADDR"); return EXIT_FAILURE; }
memset(&addr,0,sizeof(struct sockaddr_in));
addr.sin_family=AF_INET;
addr.sin_port=htons(portNumber);
addr.sin_addr.s_addr=htonl(INADDR_ANY);
RESTART_SYSCALL(r,bind(listenFd,(const struct sockaddr *)&addr, sizeof(struct sockaddr_in)));

if(r==-1) { perror("bind"); return EXIT_FAILURE; }
if(listen(listenFd,5)==-1) { perror("listen"); return EXIT_FAILURE; }
//
// Créer une socket TCP écoutant sur le port indiqué en ligne de commande.
//

//---- initialise a mutex to protect the shared variables ----
pthread_mutex_init(&mtx,NULL);

for(;;)
  {
  //---- accept new connection ----
  //
  struct sockaddr_in fromAddr;
  socklen_t len=sizeof(fromAddr);
  int * dialogSocket=(int *) malloc(sizeof(int));
  * dialogSocket=accept(listenFd,(struct sockaddr *)&fromAddr,&len);
  //
  // Attendre la connexion d'un nouveau client TCP.
  //

  //---- start a new dialog thread ----
  //
  pthread_t th;
  pthread_create(&th,NULL,dialogThread,(void*)dialogSocket);
  //
  // Démarrer un thread qui assurera le dialogue avec ce nouveau client dans
  // la fonction ``dialogThread()''.
  //

  //---- start counting elapsed time from the first connexion ----
  if(startTime<0.0) startTime=getTime();
  }

//---- close resources ----
pthread_mutex_destroy(&mtx);
free((void *)slices);
//
close(listenFd);
//
// fermer la socket d'écoute (jamais atteint ici, pas grave).
//

return 0;
}

//----------------------------------------------------------------------------
