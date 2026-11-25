/* Embute o pacote de assets (gerado por build_release.ps1) dentro do executável.
 * O caminho do .incbin é procurado nas pastas passadas com -Wa,-I (build/release). */
    .section .rdata,"dr"
    .balign 16
    .globl iara_pack_start
iara_pack_start:
    .incbin "iara_assets.pak"
    .globl iara_pack_end
iara_pack_end:
    .byte 0
