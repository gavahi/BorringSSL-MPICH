/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* added by abu naser */
//#include <sodium.h> // added by abu naser
//#include "/home/mpiuser/boringssl/include/openssl/evp.h"
//#include "/home/mpiuser/boringssl/include/openssl/aes.h"
//#include "/home/mpiuser/boringssl/include/openssl/err.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/aead.h>
char ciphertext[4194304+18];
EVP_AEAD_CTX *ctx = NULL; 

 //char nonce[16];
 //char ADDITIONAL_DATA[10];
  //int ADDITIONAL_DATA_LEN=6;
  //char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};
  //char ADDITIONAL_DATA[6] = {'1','2','3','4','5','6'};
//unsigned int ADDITIONAL_DATA_LEN;
//unsigned char nonce[12];
//unsigned char ADDITIONAL_DATA[6];
/* end of add */

/* -- Begin Profiling Symbol Block for routine MPI_Send */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Send = PMPI_Send
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Send  MPI_Send
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Send as PMPI_Send
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) __attribute__((weak,alias("PMPI_Send")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Send
#define MPI_Send PMPI_Send

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Send

/*@
    MPI_Send - Performs a blocking send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
This routine may block until the message is received by the destination 
process.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

.seealso: MPI_Isend, MPI_Bsend
@*/
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Send";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_SEND);
    
    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
	    MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
	    
	    /* Validate datatype handle */
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    
	    /* Validate datatype object */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
	    }
	    
	    /* Validate buffer */
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr, 
			  MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr == NULL)
    {
	goto fn_exit;
    }

    /* If a request was returned, then we need to block until the request 
       is complete */
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPID_Request_release(request_ptr);
    
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_send", 
	    "**mpi_send %p %d %D %i %t %C", buf, count, datatype, dest, tag, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*Added by Abu Naser, june 11 */
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    //char * temp_buf;
    //temp_buf = (char *) MPIU_Malloc((count) * sizeof(datatype));

    /* copy original data to temp buffer */
   // memcpy(temp_buf,buf,sizeof(datatype) * count);

   // for(i=0;i<count;i++){
   //     printf("temp_buf=%x,%c\n",*temp_buf,*temp_buf);fflush(stdout);
   // }
   
    //int var = sodium_init();
    
    //unsigned char ciphertext[MESSAGE_LEN + crypto_aead_aes256gcm_ABYTES];
   // char * ciphertext;
    //ciphertext=(char *) MPIU_Malloc(((count) * sizeof(datatype))  + 16);
    int ciphertext_len=0; 
     int ADDITIONAL_DATA_LEN = 6;
     char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};
     char ADDITIONAL_DATA[6] = {'1','2','3','4','5','6'};
    //unsigned char decrypted[MESSAGE_LEN];
	//unsigned long long decrypted_len;
    //unsigned char * MESSAGE; 
    int * val;
    unsigned char * c;
    int   max_out_len = 64 + count;

    /*EVP_AEAD_CTX *enctx = NULL; 
    enctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);*/ 
   
    //crypto_aead_aes256gcm_encrypt(ciphertext, &ciphertext_len,
    //                          buf, count,
    //                          ADDITIONAL_DATA, ADDITIONAL_DATA_LEN,
    //                          NULL, nonce, key);
     if(!EVP_AEAD_CTX_seal(ctx, ciphertext,
                                     &ciphertext_len, max_out_len,
                                     nonce, 12,
                                     buf,  count,
                                     ADDITIONAL_DATA, ADDITIONAL_DATA_LEN)
            ){
                //printf("Encryption done: EVP_aead_aes_256_gcm, ciphertext length is %lu count=%d\n",ciphertext_len,count);fflush(stdout);
                //for(j=0;j<ciphertext_len;j++)
                //    printf("%02x ",(unsigned int)ciphertext[j]);
                //printf("\n");    

              printf("error in encryption\n");fflush(stdout);
            }
            //else{
              //  printf("encryption success\n");
           // }
   
    
	//printf("send: sending cipher text length is = %d\n",ciphertext_len);
    //fflush(stdout);
	///for(i=0;i<ciphertext_len;i++){
	//	printf("%x ",*((unsigned char *)(ciphertext+i)));
    //    fflush(stdout);
    //}
	//printf("\n"); fflush(stdout);
    
 
    mpi_errno=MPI_Send(ciphertext, ciphertext_len, datatype, dest, tag, comm);

    //memcpy(buf,temp_buf,sizeof(datatype)*count);
    //MPIU_Free(temp_buf);
    //MPIU_Free(ciphertext);
    //EVP_AEAD_CTX_free(enctx);
    return mpi_errno;
}

void init_crypto(){
    int i;  
    unsigned char key [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
    //ADDITIONAL_DATA_LEN = 6;

   // for(i=0;i<12;i++)
   //   nonce[i]=(unsigned char)('a'+i);
   // for(i=0;i<6;i++)
   //   ADDITIONAL_DATA[i]=(unsigned char)('a'+i);
     //strcpy(nonce,"012345678912");  
    // strcpy(ADDITIONAL_DATA,"123456"); 
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm_siv(),
                            key,
                            16, 0);
    return;                        
}
/*End of adding */
