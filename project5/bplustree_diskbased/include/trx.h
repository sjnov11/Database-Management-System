#ifndef __TRX_H__
#define __TRX_H__

int trx_seq_num;

int begin_transaction();
int commit_transaction();
int abort_transaction();

#endif
