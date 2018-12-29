/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Copyright (c) 2001-2018, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 *
 */

#include "mpiimpl.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/aead.h>

unsigned char ciphertext[4194304+18];
EVP_AEAD_CTX *ctx = NULL;
unsigned char key [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
unsigned char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};  
int nonceCounter; 


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
#if defined(CHANNEL_MRAIL)
        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS)
        {
            goto fn_fail;
        }
#endif /* defined(CHANNEL_MRAIL) */
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

#if 0
int MPI_SEC_Send_primery(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
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
    // int ADDITIONAL_DATA_LEN = 6;
     char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};
    // char ADDITIONAL_DATA[6] = {'1','2','3','4','5','6'};
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
                                     NULL, 0)
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
#endif

/* variable nonce implementation */
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));
    
    /* Set the nonce in send_ciphertext */
    nonceCounter++;
    memset(ciphertext, 0, 8);
    ciphertext[8] = (nonceCounter >> 24) & 0xFF;
    ciphertext[9] = (nonceCounter >> 16) & 0xFF;
    ciphertext[10] = (nonceCounter >> 8) & 0xFF;
    ciphertext[11] = nonceCounter & 0xFF;

    /* ciphertext's first 12 byte is now nonce. so cipher will start from */
    /* ciphertext+12. And nonce is from ciphertext                   */    
    if(!EVP_AEAD_CTX_seal(ctx, ciphertext+12,
                         &ciphertext_len, max_out_len,
                         ciphertext, 12,
                         buf,  (unsigned long)(count*sendtype_sz),
                        NULL, 0)){
              printf("error in encryption\n");
              fflush(stdout);
        }
    
    /* Sending additional 12 bytes for nonce */  
	mpi_errno = MPI_Send(ciphertext, ciphertext_len+12, MPI_CHAR, dest, tag, comm);

    return mpi_errno;
}

/* Fixed nonce implementation */
#if 0
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));
    
    if(!EVP_AEAD_CTX_seal(ctx, ciphertext,
                         &ciphertext_len, max_out_len,
                         nonce, 12,
                         buf,  (unsigned long)(count*sendtype_sz),
                        NULL, 0)){
              printf("error in encryption\n");
              fflush(stdout);
        }
    
	mpi_errno = MPI_Send(ciphertext, ciphertext_len, MPI_CHAR, dest, tag, comm);

    return mpi_errno;
}
#endif

void init_crypto(){
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);
    return;                        
}


void init_boringssl_256_siv(){
    //unsigned char key_boringssl_siv_32 [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);
	printf("\n\t\t****** Secure Run with BoringSSL  256  GCM-SIV ********\n");
    return;                        
}

void init_boringssl_128_siv(){
   // unsigned char key_boringssl_siv_16 [16] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm_siv(),
                            key,
                            16, 0);
	printf("\n\t\t****** Secure Run with BoringSSL  128  GCM-SIV ********\n");
    return;                        
}

void init_boringssl_128(){
   // unsigned char key_boringssl_16 [16] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm(),
                            key,
                            16, 0);
    printf("\n\t\t****** Secure Run with BoringSSL  128  GCM ********\n");
	return;                        
}

void init_boringssl_256(){
    //unsigned char key_boringssl_32 [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm(),
                            key,
                            32, 0);
    printf("\n\t\t****** Secure Run with BoringSSL  256  GCM ********\n");
	return;                        
}

/*End of adding */
