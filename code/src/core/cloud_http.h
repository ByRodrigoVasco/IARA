#ifndef CLOUD_HTTP_H
#define CLOUD_HTTP_H

#include "cloud.h"

/*
 * Transporte real da nuvem: HTTPS pelo WinHTTP do Windows. A URL e a chave pública entram na
 * compilação (-DIARA_NUVEM_URL e -DIARA_NUVEM_CHAVE, postas pelo tools/release/build_release.ps1
 * a partir do tools/nuvem/nuvem.local). Só o build que as recebe precisa de -lwinhttp.
 */

/** @brief O transporte, ou NULL num build sem URL e chave (o jogo funciona só local). */
cloud_transport cloud_http_transport(void);

#endif // CLOUD_HTTP_H
