#pragma once
/* Inicialización de S.O (soporte para sockets y manejo de archivos) */
int sd_platform_init(void);
void sd_platform_cleanup(void);
/* Devuelve un timestamp monotónico en milisegundos */
double sd_platform_now_ms(void);
