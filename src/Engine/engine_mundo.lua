local ffi = require("ffi")
local bit = require("bit")
local C   = require("src.Engine.engineffi")

local Mundo = {}

pcall(ffi.cdef, [[
    typedef struct {
        int      numero; uint8_t flags; int largura; int altura;
        int      num_frames;
        struct { int coluna; int linha; } frames[32];
        float    fps; int loop; float alpha; float escala; int z_index;
    } TupiTileDef;
    typedef struct { uint8_t def_id; uint8_t _pad[3]; float tempo; int frame; } TupiTile;
    typedef struct {
        unsigned int textura_id; int atlas_larg; int atlas_alt; int colunas; int linhas;
        TupiTile *celulas; int num_defs; TupiTileDef defs[256]; int valido;
    } TupiMapa;
    typedef struct { int ok; int num_erros; char mensagem[512]; } TupiMapaValidacao;
    typedef struct { float x, y, largura, altura; int flags; } TupiTileHitbox;
    TupiMapa      *tupi_mapa_criar        (int colunas, int linhas);
    void           tupi_mapa_destruir     (TupiMapa *mapa);
    void           tupi_mapa_set_atlas    (TupiMapa *mapa, unsigned int tex_id, int larg, int alt);
    int            tupi_mapa_registrar_def(TupiMapa *mapa, const TupiTileDef *def);
    void           tupi_mapa_set_grade    (TupiMapa *mapa, const uint8_t *ids, int n);
    void           tupi_mapa_atualizar    (TupiMapa *mapa, float dt);
    void           tupi_mapa_desenhar     (TupiMapa *mapa, int z_base);
    TupiTileHitbox tupi_mapa_hitbox_tile  (const TupiMapa *mapa, int col, int lin);
    int            tupi_mapa_tile_em_ponto(const TupiMapa *mapa, float px, float py);
    int            tupi_mapa_flags_tile   (const TupiMapa *mapa, int col, int lin);
    int            tupi_mapa_validar      (TupiMapa *mapa, TupiMapaValidacao *out);
]])

local SOLIDO  = 0x01
local TRIGGER = 0x04

local InstMeta = {}
InstMeta.__index = InstMeta

local function novaInstancia()
    return setmetatable({
        _larg_tile=0, _alt_tile=0, _colunas=0, _linhas=0,
        _atlas_path=nil, _atlas_spr=nil, _mapa_c=nil, _valido=false, _array=nil,
        _defs={},
        _flags_lua={},
        _id_c_map={},
    }, InstMeta)
end

function InstMeta:criarMapa(lm, am, lt, at)
    assert(lm>0 and am>0 and lt>0 and at>0, "[Mapa] criarMapa: argumentos inválidos")
    self._colunas=lm; self._linhas=am; self._larg_tile=lt; self._alt_tile=at
    self._defs={}; self._flags_lua={}; self._id_c_map={}; self._valido=false; self._mapa_c=nil
end

function InstMeta:atlas(caminho)
    assert(type(caminho)=="string", "[Mapa] atlas: caminho deve ser string")
    self._atlas_path = caminho
end

-- Define flags (solido, trigger, passagem) e opções visuais de um tile
function InstMeta:flags(tile_id, opts)
    assert(type(tile_id)=="number" and tile_id >= 1,
        "[Mapa] flags: tile_id inválido: "..tostring(tile_id))
    local d = self._defs[tile_id]
    if not d then
        d = {
            frames = {{coluna=0, linha=0}},
            fps=0, loop=false, alpha=1.0, escala=1.0, z=0,
            flags=0,
            _auto=true, -- UV será calculado em build()
        }
        self._defs[tile_id] = d
    end
    opts = opts or {}
    local f = 0
    if opts.solido   or opts.colide   then f = bit.bor(f, 0x01) end
    if opts.passagem                  then f = bit.bor(f, 0x02) end
    if opts.trigger                   then f = bit.bor(f, 0x04) end
    d.flags = f
    self._flags_lua[tile_id] = f
    if opts.z     then d.z     = opts.z     end
    if opts.alpha then d.alpha = opts.alpha end
    if opts.escala then d.escala = opts.escala end
    return self
end

-- Define animação de um tile: lista de frames {c, l}, fps e loop
function InstMeta:frames(tile_id, frames_t, fps, loop)
    assert(type(tile_id)=="number" and tile_id >= 1, "[Mapa] frames: tile_id inválido")
    assert(type(frames_t)=="table" and #frames_t > 0, "[Mapa] frames: lista de frames vazia")
    local d = self._defs[tile_id]
    if not d then
        self:flags(tile_id, {})
        d = self._defs[tile_id]
    end
    local lista = {}
    for _, fr in ipairs(frames_t) do
        local col = fr.c or fr[1] or 0
        local lin = fr.l or fr[2] or 0
        table.insert(lista, {coluna=col, linha=lin})
    end
    d.frames = lista
    d.fps    = tonumber(fps) or 0
    d.loop   = loop ~= false
    if d.fps > 0 then
        d.flags = bit.bor(d.flags, 0x08)
        self._flags_lua[tile_id] = d.flags
    end
    return self
end

function InstMeta:carregarArray(arr)
    assert(type(arr)=="table" and #arr > 0, "[Mapa] carregarArray: array vazio")
    self._array = arr
end

-- Cria def placeholder para tiles sem def; UV calculado depois em build()
local function _garantirDef(self, tile_id)
    if self._defs[tile_id] then return end
    self._defs[tile_id] = {
        frames = {{coluna=0, linha=0}},
        fps=0, loop=false, alpha=1.0, escala=1.0, z=0, flags=0,
        _auto=true,
    }
    self._flags_lua[tile_id] = 0
end

-- Monta o mapa: carrega atlas, registra defs e envia grade para o C
function InstMeta:build()
    assert(self._colunas > 0, "[Mapa] build: criarMapa não chamado")
    assert(self._atlas_path, "[Mapa] build: atlas não definido")
    assert(self._array, "[Mapa] build: carregarArray não chamado")

    local total = #self._array
    assert(total == self._colunas * self._linhas,
        string.format("[Mapa] array tem %d elementos, esperado %d (%dx%d)",
            total, self._colunas*self._linhas, self._colunas, self._linhas))

    local usados = {}
    for _, tid in ipairs(self._array) do
        if tid and tid > 0 then usados[tid] = true end
    end

    for tid in pairs(usados) do _garantirDef(self, tid) end

    self._atlas_spr = C.tupi_sprite_carregar(self._atlas_path)
    assert(self._atlas_spr ~= nil, "[Mapa] falha ao carregar atlas: "..self._atlas_path)

    local atlas_larg  = tonumber(self._atlas_spr.largura)
    local atlas_alt   = tonumber(self._atlas_spr.altura)
    local atlas_cols  = math.max(1, math.floor(atlas_larg / self._larg_tile))

    -- Calcula UV das defs automáticas agora que temos o tamanho real do atlas
    for tid, def in pairs(self._defs) do
        if def._auto then
            local id0 = tid - 1
            def.frames[1].coluna = id0 % atlas_cols
            def.frames[1].linha  = math.floor(id0 / atlas_cols)
            def._auto = nil
        end
    end

    self._mapa_c = C.tupi_mapa_criar(self._colunas, self._linhas)
    assert(self._mapa_c ~= nil, "[Mapa] tupi_mapa_criar retornou nil")

    C.tupi_mapa_set_atlas(self._mapa_c, tonumber(self._atlas_spr.textura), atlas_larg, atlas_alt)

    local ids_ordenados = {}
    for tid in pairs(usados) do table.insert(ids_ordenados, tid) end
    table.sort(ids_ordenados)

    for _, tid in ipairs(ids_ordenados) do
        local def = self._defs[tid]
        local cd  = ffi.new("TupiTileDef")
        cd.numero     = tid
        cd.flags      = def.flags
        cd.largura    = self._larg_tile
        cd.altura     = self._alt_tile
        cd.num_frames = math.min(#def.frames, 32)
        cd.fps        = def.fps
        cd.loop       = def.loop and 1 or 0
        cd.alpha      = def.alpha
        cd.escala     = def.escala
        cd.z_index    = def.z
        for i, fr in ipairs(def.frames) do
            if i > 32 then break end
            cd.frames[i-1].coluna = fr.coluna
            cd.frames[i-1].linha  = fr.linha
        end
        local id_c = C.tupi_mapa_registrar_def(self._mapa_c, cd)
        assert(id_c >= 0, "[Mapa] limite de 255 tipos de tile atingido")
        self._id_c_map[tid] = id_c + 1
    end

    local grade = ffi.new("uint8_t[?]", total)
    for i, tid in ipairs(self._array) do
        if not tid or tid == 0 then
            grade[i-1] = 0
        else
            local id_c = self._id_c_map[tid]
            assert(id_c, "[Mapa] tile_id "..tostring(tid).." usado na grade mas não registrado")
            grade[i-1] = id_c
        end
    end
    C.tupi_mapa_set_grade(self._mapa_c, grade, total)

    local val = ffi.new("TupiMapaValidacao")
    if C.tupi_mapa_validar(self._mapa_c, val) == 0 then
        C.tupi_mapa_destruir(self._mapa_c); self._mapa_c = nil
        error("[Mapa] Validação falhou:\n"..ffi.string(val.mensagem))
    end
    self._valido = true
end

function InstMeta:atualizar(dt)
    if self._valido then C.tupi_mapa_atualizar(self._mapa_c, dt or 0) end
end

-- Desenha o mapa com inset de 0.01px nas UVs para evitar texture bleeding
function InstMeta:desenhar(z)
    if not self._valido then return end

    local spr    = self._atlas_spr
    local alarg  = tonumber(spr.largura)
    local aalt   = tonumber(spr.altura)
    local tw     = self._larg_tile
    local th     = self._alt_tile
    local cols   = self._colunas
    local lins   = self._linhas
    local arr    = self._array
    local q      = ffi.new("float[16]")
    local tex    = tonumber(spr.textura)
    local z_base = z or 0
    local atlas_cols = math.max(1, math.floor(alarg / tw))

    for lin = 0, lins - 1 do
        for col = 0, cols - 1 do
            local tid = arr[lin * cols + col + 1]
            if tid and tid > 0 then
                local def = self._defs[tid]
                local fr  = def and def.frames[1]
                local tc  = fr and fr.coluna or 0
                local tl  = fr and fr.linha  or 0

                local px0 = col * tw
                local py0 = lin * th
                local px1 = px0 + tw
                local py1 = py0 + th

                -- UV com inset para evitar bleeding
                local px0_atlas = tc * tw
                local py0_atlas = tl * th
                local px1_atlas = px0_atlas + tw
                local py1_atlas = py0_atlas + th
                local eps = 0.01
                local u0 = (px0_atlas + eps) / alarg
                local v0 = (py0_atlas + eps) / aalt
                local u1 = (px1_atlas - eps) / alarg
                local v1 = (py1_atlas - eps) / aalt

                q[0]=px0; q[1]=py0; q[2]=u0;  q[3]=v0
                q[4]=px1; q[5]=py0; q[6]=u1;  q[7]=v0
                q[8]=px0; q[9]=py1; q[10]=u0; q[11]=v1
                q[12]=px1;q[13]=py1;q[14]=u1; q[15]=v1

                local z_tile = z_base + (def and def.z or 0)
                C.tupi_objeto_enviar_batch_raw(tex, q, 1, 1, 1,
                    def and def.alpha or 1.0, z_tile)
            end
        end
    end
    C.tupi_batch_desenhar()
end

-- Retorna flags do tile em (col, lin); 0 se fora do mapa
function InstMeta:_flagsEmGrade(col, lin)
    if col < 0 or col >= self._colunas or lin < 0 or lin >= self._linhas then return 0 end
    local tid = self._array[lin * self._colunas + col + 1]
    if not tid or tid == 0 then return 0 end
    return self._flags_lua[tid] or 0
end

function InstMeta:isSolido(col, lin)
    return bit.band(self:_flagsEmGrade(col, lin), SOLIDO) ~= 0
end
function InstMeta:isTrigger(col, lin)
    return bit.band(self:_flagsEmGrade(col, lin), TRIGGER) ~= 0
end
function InstMeta:isPassagem(col, lin)
    return bit.band(self:_flagsEmGrade(col, lin), 0x02) ~= 0
end

function InstMeta:hitboxTile(col, lin)
    if not self._valido then return nil end
    local hb = C.tupi_mapa_hitbox_tile(self._mapa_c, col, lin)
    local f  = self:_flagsEmGrade(col, lin)
    return {
        x=tonumber(hb.x), y=tonumber(hb.y),
        largura=tonumber(hb.largura), altura=tonumber(hb.altura),
        flags=f, solido=bit.band(f,SOLIDO)~=0, trigger=bit.band(f,TRIGGER)~=0,
    }
end

function InstMeta:tileEmPonto(px, py)
    if not self._valido then return 0 end
    return tonumber(C.tupi_mapa_tile_em_ponto(self._mapa_c, px, py))
end

function InstMeta:tileIdEmGrade(col, lin)
    if col < 0 or col >= self._colunas or lin < 0 or lin >= self._linhas then return 0 end
    return self._array[lin * self._colunas + col + 1] or 0
end

function InstMeta:definicaoEmGrade(col, lin)
    local tid = self:tileIdEmGrade(col, lin)
    if tid == 0 then return nil end
    return self._defs[tid]
end

function InstMeta:definicaoEmPonto(px, py)
    return self:definicaoEmGrade(
        math.floor(px / self._larg_tile),
        math.floor(py / self._alt_tile))
end

function InstMeta:destruir()
    if self._mapa_c   then C.tupi_mapa_destruir(self._mapa_c);      self._mapa_c   = nil end
    if self._atlas_spr then C.tupi_sprite_destruir(self._atlas_spr); self._atlas_spr = nil end
    self._valido = false
end

local Mapa = {}
function Mapa.novo() return novaInstancia() end
function Mapa.carregar(modulo)
    assert(type(modulo)=="string" and modulo~="")
    local r = require(modulo)
    assert(type(r)=="table"); return r
end

Mundo.mapa = Mapa

-- ─── MUNDOS ──────────────────────────────────────────────────────────────────
local Mundos = {}
local _arquivo_atual=nil; local _nome_atual=nil; local _cache={}; local _inst_atual=nil
local _ao_sair=nil; local _ao_entrar=nil

local function _listarChaves(t)
    local lista={}
    for k in pairs(t) do if type(k)=="string" then table.insert(lista,k) end end
    return "{"..table.concat(lista,", ").."}"
end

-- Carrega e cacheia a instância do mapa; reutiliza se já foi carregada
local function _obterInstancia(arquivo, nome)
    assert(type(arquivo)=="string" and arquivo~="" and type(nome)=="string" and nome~="")
    _cache[arquivo] = _cache[arquivo] or {}
    if _cache[arquivo][nome] then return _cache[arquivo][nome] end
    local tabela = Mapa.carregar(arquivo)
    assert(tabela[nome], string.format(
        "[Mundos] mapa '%s' não encontrado em '%s'. Disponíveis: %s",
        nome, arquivo, _listarChaves(tabela)))
    for k,v in pairs(tabela) do
        if type(v)=="table" and v.atualizar then _cache[arquivo][k]=v end
    end
    return _cache[arquivo][nome]
end

-- Troca o mapa atual e dispara callbacks de saída/entrada
local function _aplicarTroca(arquivo, nome)
    local nova = _obterInstancia(arquivo, nome)
    if _ao_sair and _inst_atual then _ao_sair(_arquivo_atual,_nome_atual,_inst_atual) end
    _arquivo_atual=arquivo; _nome_atual=nome; _inst_atual=nova
    if _ao_entrar then _ao_entrar(_arquivo_atual,_nome_atual,_inst_atual) end
end

function Mundos.trocar(arquivo, nome)            _aplicarTroca(arquivo, nome) end
function Mundos.trocar_arquivos(_, _, arq, nome) _aplicarTroca(arq, nome) end
function Mundos.atual()      return _inst_atual end
function Mundos.infoAtual()  return _arquivo_atual, _nome_atual end
function Mundos.eAtual(a, n) return _arquivo_atual==a and _nome_atual==n end
function Mundos.precarregar(a, n) _obterInstancia(a, n) end

function Mundos.descarregar(arquivo, nome)
    if not (_cache[arquivo] and _cache[arquivo][nome]) then return end
    local inst = _cache[arquivo][nome]
    if inst == _inst_atual then
        print("[Mundos] aviso: tentativa de descarregar o mapa atual ignorada."); return
    end
    if inst.destruir then inst:destruir() end
    _cache[arquivo][nome] = nil
    if not next(_cache[arquivo]) then _cache[arquivo] = nil end
end

function Mundos.aoSair(fn)   assert(fn==nil or type(fn)=="function"); _ao_sair=fn   end
function Mundos.aoEntrar(fn) assert(fn==nil or type(fn)=="function"); _ao_entrar=fn end

function Mundos.destruirTudo()
    for arq,nomes in pairs(_cache) do
        for nome,inst in pairs(nomes) do
            if inst ~= _inst_atual and inst.destruir then inst:destruir() end
        end
    end
    _cache = {}
    if _inst_atual and _inst_atual.destruir then _inst_atual:destruir() end
    _inst_atual=nil; _arquivo_atual=nil; _nome_atual=nil
end

Mundo.mundos = Mundos

return Mundo