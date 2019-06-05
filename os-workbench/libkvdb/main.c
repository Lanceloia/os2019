#include "kvdb.h"
#include <stdlib.h>
#include <stdio.h>

void *test(void *_db) {
  kvdb_t *db = _db;
  char *str;
// code: 
  kvdb_put(db, "III","abcd");
  kvdb_put(db, "JJJ","dubgtiynvhqd");
  kvdb_put(db, "DDD","advnuiczkupekqcpvta");
  kvdb_put(db, "KKK","asx");
  kvdb_put(db, "NNN","cagfduoinpmnn");
  kvdb_put(db, "OOO","fyernerrynqewzdiogus");
  kvdb_put(db, "AAA","wxdfpokzjcbhfcdmpxuhkklwefgwkhdgfgnwxyvicwpjzuxot");
  kvdb_put(db, "BBB","jxhtblnxkjilpozvnluvkqhxhmypvsegrlcs");
  kvdb_put(db, "CCC","asx");
  str = kvdb_get(db, "OOO"); printf("%s\n", str == NULL ? "(null)" : str);
  str = kvdb_get(db, "III"); printf("%s\n", str == NULL ? "(null)" : str);
  kvdb_put(db, "DDD","xpbovskxhgvufnggvnuobhweczdxghlewourgeonmm");
  kvdb_put(db, "CCC","xfuhxaikupjnbtusdo");
  kvdb_put(db, "FFF","unttjwyulroniqcefnkzqzjfjjrainjcceylc");
  str = kvdb_get(db, "FFF"); printf("%s\n", str == NULL ? "(null)" : str);
  str = kvdb_get(db, "BBB"); printf("%s\n", str == NULL ? "(null)" : str);
  str = kvdb_get(db, "NNN"); printf("%s\n", str == NULL ? "(null)" : str);
  kvdb_put(db, "LLL","gaygmwkgcthkfmghoxhwuiswvgadqzxwb");
  kvdb_put(db, "LLL","meyycqtynztikfgpgwxtasrpprxddm");
  kvdb_put(db, "NNN","bfvldqaoiixdmvrorercadtkbbswzwjbdgmgymwgwullrdbiju");
  kvdb_put(db, "PPP","gmi"); 
  kvdb_put(db, "LLL","wrpubvfxenkqwwtwbqddflqqtdznowllpahqxmnbbatxwo");
  kvdb_put(db, "DDD","vmysxnjfvsierdsuposevjqkepgxabzynzsmpdtmvdq");
  kvdb_put(db, "FFF","erunusyywpewfcuglthcshonijnppkp");
  str = kvdb_get(db, "DDD"); printf("%s\n", str == NULL ? "(null)" : str);
  kvdb_put(db, "HHH","gpwcimhxmtmckdauxwbnldnbzregipireetpsc");
  kvdb_put(db, "CCC","gxzgwbzgxqusloy");
  kvdb_put(db, "OOO","yiylslsiuvfxcdjxrqbolgjacrlqnunmfozzbriwonttseqlutcfblgfftwsq");
  kvdb_put(db, "LLL","ehnsnqzitnkherkhhmehyaeet");
  str = kvdb_get(db, "NNN"); printf("%s\n", str == NULL ? "(null)" : str);
  kvdb_put(db, "III","uoxtdhsobpfuzvospenzqzsoqzqgkizgy");
  kvdb_put(db, "KKK","fosrwukfgrtmzhlgfhrydmgfpxfmvgi");
  kvdb_put(db, "FFF","hklybdcwmuhsmppoqqmgqeshjotfrlwawhbxkfvwceootfckw");
  str = kvdb_get(db, "CCC"); printf("%s\n", str == NULL ? "(null)" : str);
  kvdb_put(db, "CCC","vust"); 
  kvdb_put(db, "MMM","fvehljrsbdvsjgbsuccwbynnvqdeiqupnyyyjp");
  kvdb_put(db, "DDD","xeybkjrcewbrlzewoedhdrwgs");
  kvdb_put(db, "FFF","wfmauapgcdjguajdzbkhpnbnvjaladtykfzgiqmktxspybszecitsmipvlcxov");
// #define EXIT
#ifdef EXIT
  exit(1);
#endif
  return NULL;
}

#define THREADS 1

int main(int argc, char *argv[]) {
  kvdb_t *db = malloc(sizeof(kvdb_t));
  if(db == NULL) { panic("malloc failed. \n"); return 1; }

  if(kvdb_open(db, argv[1])) { panic("cannot open. \n"); return 1; }

  pthread_t pt[THREADS];
  for(int i = 0; i < THREADS; i ++) {
    pthread_create(&pt[i], NULL, test, db);

  }
  for(int i = 0; i < THREADS; i ++) {
    pthread_join(pt[i], NULL);
  }

  if(kvdb_close(db)) { panic("cannot close. \n"); return 1; }

  return 0;
}