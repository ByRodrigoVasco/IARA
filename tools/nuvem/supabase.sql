-- IARA: saves e rankings compartilhados entre os notebooks (Supabase).
--
-- Como montar o banco: tools/nuvem/LEIA-ME.md. O arquivo e idempotente (pode rodar de novo).
--
-- Seguranca: as tabelas tem RLS sem nenhuma policy e o anon nao tem privilegio nelas. O jogo so
-- chama as funcoes abaixo (security definer). A senha de apagar saves fica em iara_config, so
-- como hash SHA-256, e NAO esta neste arquivo.
--
-- Os pesos de iara_pontos sao os mesmos de code/src/survival/sv_ranking.h.

-- ---------------------------------------------------------------------------
-- Tabelas
-- ---------------------------------------------------------------------------

create table if not exists public.iara_saves (
    modo          text        not null,
    nome          text        not null,
    nome_chave    text        not null generated always as (lower(nome)) stored,
    conteudo      text        not null default '',
    versao        integer     not null default 0,
    criado_em     timestamptz not null default now(),
    atualizado_em timestamptz not null default now(),
    em_uso_por    text,
    em_uso_ate    timestamptz,
    constraint iara_saves_pkey primary key (modo, nome_chave),
    constraint iara_saves_modo_check check (modo in ('normal', 'sobrevivencia', 'dupla')),
    constraint iara_saves_nome_check check (nome ~ '^[A-Za-z0-9 _-]{1,20}$'),
    constraint iara_saves_conteudo_check check (length(conteudo) <= 65536)
);

create table if not exists public.iara_ranking (
    tipo          text        not null,
    nome          text        not null,
    nome_chave    text        not null generated always as (lower(nome)) stored,
    segundos      integer     not null,
    abates        integer     not null,
    nivel         integer     not null,
    chefes        integer     not null,
    personagem    integer     not null,
    venceu        boolean     not null,
    pontos        integer     not null,
    registrado_em timestamptz not null default now(),
    constraint iara_ranking_pkey primary key (tipo, nome_chave),
    constraint iara_ranking_tipo_check check (tipo in ('solo', 'dupla'))
);

create table if not exists public.iara_config (
    chave text not null,
    valor text not null,
    constraint iara_config_pkey primary key (chave)
);

alter table public.iara_saves   enable row level security;
alter table public.iara_ranking enable row level security;
alter table public.iara_config  enable row level security;

revoke all on table public.iara_saves, public.iara_ranking, public.iara_config
    from public, anon, authenticated;

-- ---------------------------------------------------------------------------
-- Funcoes
-- ---------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.iara_trava_validade()
 RETURNS interval
 LANGUAGE sql
 IMMUTABLE
AS $function$ select interval '2 minutes' $function$;

CREATE OR REPLACE FUNCTION public.iara_listar_saves(p_modo text, p_maquina text)
 RETURNS TABLE(nome text, atualizado_em timestamp with time zone, tem_progresso boolean, em_uso boolean)
 LANGUAGE sql
 STABLE SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
    select s.nome, s.atualizado_em, s.versao > 0,
           s.em_uso_por is not null and s.em_uso_por <> p_maquina and s.em_uso_ate > now()
    from iara_saves s
    where s.modo = p_modo
    order by s.atualizado_em desc
$function$;

CREATE OR REPLACE FUNCTION public.iara_criar_save(p_modo text, p_nome text)
 RETURNS text
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
begin
    insert into iara_saves (modo, nome) values (p_modo, p_nome);
    return 'ok';
exception
    when unique_violation then return 'existe';
    when check_violation then return 'invalido';
end $function$;

CREATE OR REPLACE FUNCTION public.iara_abrir_save(p_modo text, p_nome text, p_maquina text)
 RETURNS TABLE(status text, conteudo text, versao integer)
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
#variable_conflict use_column
declare
    s iara_saves%rowtype;
begin
    select * into s from iara_saves
    where modo = p_modo and nome_chave = lower(p_nome)
    for update;

    if not found then
        return query select 'nao_existe'::text, ''::text, 0;
        return;
    end if;

    if s.em_uso_por is not null and s.em_uso_por <> p_maquina and s.em_uso_ate > now() then
        return query select 'em_uso'::text, ''::text, s.versao;
        return;
    end if;

    update iara_saves
    set em_uso_por = p_maquina, em_uso_ate = now() + iara_trava_validade()
    where modo = p_modo and nome_chave = lower(p_nome);

    return query select 'ok'::text, s.conteudo, s.versao;
end $function$;

CREATE OR REPLACE FUNCTION public.iara_gravar_save(p_modo text, p_nome text, p_maquina text, p_conteudo text, p_versao_base integer)
 RETURNS TABLE(status text, versao integer)
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
#variable_conflict use_column
declare
    s iara_saves%rowtype;
begin
    select * into s from iara_saves
    where modo = p_modo and nome_chave = lower(p_nome)
    for update;

    if not found then
        insert into iara_saves (modo, nome, conteudo, versao, em_uso_por, em_uso_ate)
        values (p_modo, p_nome, p_conteudo, 1, p_maquina, now() + iara_trava_validade());
        return query select 'ok'::text, 1;
        return;
    end if;

    if s.em_uso_por is not null and s.em_uso_por <> p_maquina and s.em_uso_ate > now() then
        return query select 'em_uso'::text, s.versao;
        return;
    end if;

    if s.versao <> p_versao_base then
        return query select 'conflito'::text, s.versao;
        return;
    end if;

    update iara_saves
    set conteudo = p_conteudo, versao = s.versao + 1, atualizado_em = now(),
        em_uso_por = p_maquina, em_uso_ate = now() + iara_trava_validade()
    where modo = p_modo and nome_chave = lower(p_nome);

    return query select 'ok'::text, s.versao + 1;
end $function$;

CREATE OR REPLACE FUNCTION public.iara_renovar_save(p_modo text, p_nome text, p_maquina text)
 RETURNS text
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
begin
    update iara_saves
    set em_uso_por = p_maquina, em_uso_ate = now() + iara_trava_validade()
    where modo = p_modo and nome_chave = lower(p_nome)
      and (em_uso_por is null or em_uso_por = p_maquina or em_uso_ate <= now());
    return case when found then 'ok' else 'perdido' end;
end $function$;

CREATE OR REPLACE FUNCTION public.iara_liberar_save(p_modo text, p_nome text, p_maquina text)
 RETURNS text
 LANGUAGE sql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
    update iara_saves set em_uso_por = null, em_uso_ate = null
    where modo = p_modo and nome_chave = lower(p_nome) and em_uso_por = p_maquina;
    select 'ok'::text;
$function$;

CREATE OR REPLACE FUNCTION public.iara_apagar_save(p_modo text, p_nome text, p_maquina text, p_senha text)
 RETURNS text
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
declare
    s iara_saves%rowtype;
    hash text;
begin
    select valor into hash from iara_config where chave = 'senha_apagar';
    if hash is null or hash <> encode(sha256(convert_to(p_senha, 'UTF8')), 'hex') then
        perform pg_sleep(1);   -- Atrasa quem tenta adivinhar a senha
        return 'senha';
    end if;

    select * into s from iara_saves where modo = p_modo and nome_chave = lower(p_nome) for update;
    if not found then return 'nao_existe'; end if;
    if s.em_uso_por is not null and s.em_uso_por <> p_maquina and s.em_uso_ate > now() then
        return 'em_uso';
    end if;

    delete from iara_saves where modo = p_modo and nome_chave = lower(p_nome);
    if p_modo in ('sobrevivencia', 'dupla') then
        delete from iara_ranking
        where tipo = case p_modo when 'dupla' then 'dupla' else 'solo' end
          and nome_chave = lower(p_nome);
    end if;
    return 'ok';
end $function$;

CREATE OR REPLACE FUNCTION public.iara_pontos(p_segundos integer, p_abates integer, p_chefes integer, p_venceu boolean)
 RETURNS integer
 LANGUAGE sql
 IMMUTABLE
AS $function$
    select p_abates * 10 + p_segundos * 5 + p_chefes * 500 + case when p_venceu then 2000 else 0 end
$function$;

CREATE OR REPLACE FUNCTION public.iara_registrar_partida(p_tipo text, p_nome text, p_segundos integer, p_abates integer, p_nivel integer, p_chefes integer, p_personagem integer, p_venceu boolean)
 RETURNS integer
 LANGUAGE plpgsql
 SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
declare
    novo_pontos integer;
    atual iara_ranking%rowtype;
    meus_pontos integer;
    meus_abates integer;
    meus_segundos integer;
    meu_registro timestamptz;
begin
    -- Limites do jogo (code/src/survival/sv_rules.h): a partida dura 10 min (600 s) e tem 5 chefes.
    -- Os abates têm folga larga: o ritmo normal é de 2 a 5 por segundo.
    if p_tipo not in ('solo', 'dupla')
       or p_nome !~ '^[A-Za-z0-9 _-]{1,20}$'
       or p_segundos not between 0 and 600
       or p_chefes not between 0 and 5
       or p_abates < 0 or p_abates > (p_segundos + 1) * 30
       or p_nivel not between 0 and 1000
       or p_personagem not between 0 and 255 then
        return 0;
    end if;

    novo_pontos := iara_pontos(p_segundos, p_abates, p_chefes, p_venceu);

    select * into atual from iara_ranking
    where tipo = p_tipo and nome_chave = lower(p_nome)
    for update;

    -- Mesmo critério de sv_ranking_better: pontos, depois abates, depois tempo
    if not found then
        insert into iara_ranking (tipo, nome, segundos, abates, nivel, chefes, personagem, venceu, pontos)
        values (p_tipo, p_nome, p_segundos, p_abates, p_nivel, p_chefes, p_personagem, p_venceu, novo_pontos);
    elsif (novo_pontos, p_abates, p_segundos) > (atual.pontos, atual.abates, atual.segundos) then
        update iara_ranking
        set nome = p_nome, segundos = p_segundos, abates = p_abates, nivel = p_nivel, chefes = p_chefes,
            personagem = p_personagem, venceu = p_venceu, pontos = novo_pontos, registrado_em = now()
        where tipo = p_tipo and nome_chave = lower(p_nome);
    end if;

    select r.pontos, r.abates, r.segundos, r.registrado_em
    into meus_pontos, meus_abates, meus_segundos, meu_registro
    from iara_ranking r where r.tipo = p_tipo and r.nome_chave = lower(p_nome);

    return 1 + (select count(*) from iara_ranking r
                where r.tipo = p_tipo
                  and (r.pontos, r.abates, r.segundos, meu_registro) > (meus_pontos, meus_abates, meus_segundos, r.registrado_em));
end $function$;

CREATE OR REPLACE FUNCTION public.iara_ler_ranking(p_tipo text, p_limite integer DEFAULT 200)
 RETURNS TABLE(nome text, segundos integer, abates integer, nivel integer, chefes integer, personagem integer, venceu boolean, pontos integer)
 LANGUAGE sql
 STABLE SECURITY DEFINER
 SET search_path TO 'public'
AS $function$
    select r.nome, r.segundos, r.abates, r.nivel, r.chefes, r.personagem, r.venceu, r.pontos
    from iara_ranking r
    where r.tipo = p_tipo
    order by r.pontos desc, r.abates desc, r.segundos desc, r.registrado_em asc
    limit least(greatest(p_limite, 1), 200)
$function$;

-- ---------------------------------------------------------------------------
-- Permissoes: o jogo (anon) so executa as funcoes publicas; as auxiliares, ninguem
-- ---------------------------------------------------------------------------

revoke all on function public.iara_trava_validade() from public, anon, authenticated, service_role;
revoke all on function public.iara_listar_saves(text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_listar_saves(text, text) to anon;
revoke all on function public.iara_criar_save(text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_criar_save(text, text) to anon;
revoke all on function public.iara_abrir_save(text, text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_abrir_save(text, text, text) to anon;
revoke all on function public.iara_gravar_save(text, text, text, text, integer) from public, anon, authenticated, service_role;
grant execute on function public.iara_gravar_save(text, text, text, text, integer) to anon;
revoke all on function public.iara_renovar_save(text, text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_renovar_save(text, text, text) to anon;
revoke all on function public.iara_liberar_save(text, text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_liberar_save(text, text, text) to anon;
revoke all on function public.iara_apagar_save(text, text, text, text) from public, anon, authenticated, service_role;
grant execute on function public.iara_apagar_save(text, text, text, text) to anon;
revoke all on function public.iara_pontos(integer, integer, integer, boolean) from public, anon, authenticated, service_role;
revoke all on function public.iara_registrar_partida(text, text, integer, integer, integer, integer, integer, boolean) from public, anon, authenticated, service_role;
grant execute on function public.iara_registrar_partida(text, text, integer, integer, integer, integer, integer, boolean) to anon;
revoke all on function public.iara_ler_ranking(text, integer) from public, anon, authenticated, service_role;
grant execute on function public.iara_ler_ranking(text, integer) to anon;

-- Faz a API enxergar as funcoes novas na hora
notify pgrst, 'reload schema';
