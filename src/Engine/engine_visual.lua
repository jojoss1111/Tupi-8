local ffi = require("ffi")
local C   = require("src.Engine.engineffi")

local Visual = {}

local Sprite = {}

function Sprite.carregarSprite(caminho)
    assert(type(caminho) == "string", "[Sprite] caminho deve ser string")
    local spr = C.tupi_sprite_carregar(caminho)
    if spr == nil then error("[Sprite] falha ao carregar '" .. caminho .. "'") end
    return spr
end

function Sprite.destruirSprite(spr)
    if spr ~= nil then C.tupi_sprite_destruir(spr) end
end

function Sprite.criarObjeto(x, y, z, largura, altura, wx, hy, transparencia, escala, sprite)
    assert(sprite ~= nil, "[Sprite] criarObjeto: sprite não pode ser nil")
    local obj = ffi.new("TupiObjeto[1]")
    obj[0] = C.tupi_objeto_criar(x or 0, y or 0, largura, altura, wx, hy, transparencia or 1.0, escala or 1.0, sprite)
    return { obj = obj, z = z or 0 }
end

local _espelhos = {}
local _quad_espelho = ffi.new("float[16]")

local function _chaveEspelho(w) return tostring(ffi.cast("void*", w.obj)) end

local function _enviarComEspelho(wrapper, camada)
    local e = _espelhos[_chaveEspelho(wrapper)]
    if not e then C.tupi_objeto_enviar_batch(wrapper.obj, camada); return end

    local obj = wrapper.obj[0]; local spr = obj.imagem
    local cw = tonumber(obj.largura); local ch = tonumber(obj.altura)
    local lw = tonumber(spr.largura); local lh = tonumber(spr.altura)
    local col = tonumber(obj.coluna); local lin = tonumber(obj.linha)

    -- FIX: calcula UV em pixels inteiros primeiro, depois divide —
    -- evita acumulação de erro float que cortava o último pixel (v1)
    -- nas letras com descendentes (q, p, j, g).
    local px0 = col * cw;  local py0 = lin * ch
    local px1 = px0 + cw;  local py1 = py0 + ch

    local u0 = px0 / lw;  local v0 = py0 / lh
    local u1 = px1 / lw;  local v1 = py1 / lh

    if e.h then u0, u1 = u1, u0 end
    if e.v then v0, v1 = v1, v0 end

    -- FIX: arredonda sw/sh separadamente antes de somar a x0/y0 —
    -- garante que o quad em tela tenha tamanho inteiro idêntico
    -- à área coberta pelo UV.
    local x0 = math.floor(tonumber(obj.x) + 0.5)
    local y0 = math.floor(tonumber(obj.y) + 0.5)
    local x1 = x0 + math.floor(cw * tonumber(obj.escala) + 0.5)
    local y1 = y0 + math.floor(ch * tonumber(obj.escala) + 0.5)

    _quad_espelho[0]=x0; _quad_espelho[1]=y0; _quad_espelho[2]=u0; _quad_espelho[3]=v0
    _quad_espelho[4]=x1; _quad_espelho[5]=y0; _quad_espelho[6]=u1; _quad_espelho[7]=v0
    _quad_espelho[8]=x0; _quad_espelho[9]=y1; _quad_espelho[10]=u0; _quad_espelho[11]=v1
    _quad_espelho[12]=x1; _quad_espelho[13]=y1; _quad_espelho[14]=u1; _quad_espelho[15]=v1
    C.tupi_objeto_enviar_batch_raw(
        tonumber(spr.textura),
        _quad_espelho,
        1.0, 1.0, 1.0, tonumber(obj.transparencia),
        camada
    )
end

Sprite._enviarComEspelho = _enviarComEspelho
Sprite._espelhos         = _espelhos
Sprite._chaveEspelho     = _chaveEspelho

function Sprite.desenharObjeto(w)
    if w._destruido then return end
    _enviarComEspelho(w, w.z or 0); C.tupi_batch_desenhar()
end

function Sprite.espelhar(w, vertical, horizontal)
    assert(w and w.obj, "[Sprite] espelhar: wrapper inválido")
    _espelhos[_chaveEspelho(w)] = { v = vertical == true, h = horizontal == true }
end

function Sprite.getEspelho(w)
    local e = _espelhos[_chaveEspelho(w)]
    if not e then return false, false end
    return e.h, e.v
end

function Sprite.enviarBatch(w, z)
    if w._destruido then return end; _enviarComEspelho(w, z or w.z or 0)
end

function Sprite.desenhar(w, z)
    if w._destruido then return end
    _enviarComEspelho(w, z or w.z or 0); C.tupi_batch_desenhar()
end

local _animEstado   = {}
local _animContador = 0

local function _chaveAnim(anim, w)
    return tostring(anim._id) .. ":" .. tostring(ffi.cast("void*", w.obj))
end
local function _pegarEstado(anim, w)
    local k = _chaveAnim(anim, w)
    if not _animEstado[k] then _animEstado[k] = { frame=0, tempo=0.0, terminado=false } end
    return _animEstado[k]
end

function Sprite.criarAnim(sprite, largura, altura, colunas, linhas, fps, loop)
    assert(sprite ~= nil, "[Anim] sprite não pode ser nil")
    assert(type(largura) == "number" and type(altura) == "number", "[Anim] largura/altura obrigatórios")
    assert(type(colunas) == "table" and #colunas > 0, "[Anim] colunas deve ter ao menos 1 elemento")
    assert(type(linhas)  == "table" and #linhas  > 0, "[Anim] linhas deve ter ao menos 1 elemento")
    _animContador = _animContador + 1
    local pares = {}
    for _, lin in ipairs(linhas) do
        for _, col in ipairs(colunas) do table.insert(pares, { col=col, linha=lin }) end
    end
    return { _id=_animContador, _sprite=sprite, _larg=largura, _alt=altura,
             _pares=pares, _fps=fps or 8, _loop=(loop == nil) and true or loop }
end

function Sprite.tocarAnim(anim, w, z)
    assert(anim and anim._pares, "[Anim] tocarAnim: anim inválida")
    local estado = _pegarEstado(anim, w)
    local total  = #anim._pares
    local dt     = Sprite._getDt and Sprite._getDt() or 0
    if not estado.terminado then
        estado.tempo = estado.tempo + dt
        local frameAtual = math.floor(estado.tempo / (1.0 / anim._fps))
        if anim._loop then
            estado.frame = frameAtual % total
        else
            if frameAtual >= total then estado.frame = total - 1; estado.terminado = true
            else estado.frame = frameAtual end
        end
    end
    local par = anim._pares[estado.frame + 1]
    w.obj[0].coluna = par.col; w.obj[0].linha = par.linha
    _enviarComEspelho(w, z or w.z or 0)
end

function Sprite.pararAnim(anim, w, frame, z)
    assert(anim and anim._pares, "[Anim] pararAnim: anim inválida")
    _animEstado[_chaveAnim(anim, w)] = nil
    if type(frame) == "table" then
        w.obj[0].coluna = frame[1] or 0; w.obj[0].linha = frame[2] or 0
    else
        local par = anim._pares[math.min(frame or 0, #anim._pares - 1) + 1]
        w.obj[0].coluna = par.col; w.obj[0].linha = par.linha
    end
    _enviarComEspelho(w, z or w.z or 0)
end

function Sprite.animTerminou(anim, w)
    local e = _animEstado[_chaveAnim(anim, w)]
    return e ~= nil and e.terminado
end
function Sprite.animReiniciar(anim, w) _animEstado[_chaveAnim(anim, w)] = nil end
function Sprite.animLimparObjeto(w)
    local sufixo = ":" .. tostring(ffi.cast("void*", w.obj))
    for k in pairs(_animEstado) do if k:sub(-#sufixo) == sufixo then _animEstado[k] = nil end end
end

local _posicoes_salvas = {}

function Sprite.mover(dx, dy, w)     w.obj[0].x = w.obj[0].x + (dx or 0); w.obj[0].y = w.obj[0].y + (dy or 0) end
function Sprite.teleportar(x, y, w)  w.obj[0].x = x or w.obj[0].x; w.obj[0].y = y or w.obj[0].y end
function Sprite.posicaoAtual(w)      return tonumber(w.obj[0].x), tonumber(w.obj[0].y) end

function Sprite.salvarPosicao(w)
    _posicoes_salvas[tostring(ffi.cast("void*", w.obj))] = { x=tonumber(w.obj[0].x), y=tonumber(w.obj[0].y) }
end
function Sprite.ultimaPosicao(w)
    local p = _posicoes_salvas[tostring(ffi.cast("void*", w.obj))]
    if not p then return nil, nil end; return p.x, p.y
end
function Sprite.voltarPosicao(w)
    local p = _posicoes_salvas[tostring(ffi.cast("void*", w.obj))]
    if not p then return end; w.obj[0].x = p.x; w.obj[0].y = p.y
end
function Sprite.distanciaObjetos(a, b)
    local dx = tonumber(b.obj[0].x) - tonumber(a.obj[0].x)
    local dy = tonumber(b.obj[0].y) - tonumber(a.obj[0].y)
    return math.sqrt(dx * dx + dy * dy)
end

function Sprite.moverParaAlvo(tx, ty, fator, w)
    fator = fator or 0.1
    w.obj[0].x = w.obj[0].x + (tx - w.obj[0].x) * fator
    w.obj[0].y = w.obj[0].y + (ty - w.obj[0].y) * fator
end

function Sprite.perseguir(alvo, vel, w)
    local dx = tonumber(alvo.obj[0].x) - tonumber(w.obj[0].x)
    local dy = tonumber(alvo.obj[0].y) - tonumber(w.obj[0].y)
    local dist = math.sqrt(dx*dx + dy*dy)
    if dist < 0.001 then return end
    w.obj[0].x = w.obj[0].x + (dx/dist) * vel
    w.obj[0].y = w.obj[0].y + (dy/dist) * vel
end

function Sprite.hitbox(w, x, y, ww, h, escalar)
    if w._destruido then return {x=0,y=0,largura=0,altura=0} end
    local ox = tonumber(w.obj[0].x); local oy = tonumber(w.obj[0].y)
    local fator = 1.0
    if escalar ~= false then
        local ok, e = pcall(function() return tonumber(w.obj[0].escala) end)
        if ok and e and e > 0 then fator = e end
    end
    return { x=ox+x*fator, y=oy+y*fator, largura=ww*fator, altura=h*fator }
end

function Sprite.hitboxFixa(w, x, y, ww, h) return Sprite.hitbox(w, x, y, ww, h, false) end

function Sprite.hitboxDesenhar(hb, cor, esp)
    cor = cor or {0, 1, 0, 0.6}
    local r = Sprite._aplicarCor and Sprite._aplicarCor(cor) or function() end
    C.tupi_retangulo_borda(hb.x, hb.y, hb.largura, hb.altura, esp or 1.0); r()
end

local function _moverX(dx, wA, ox, oy, w, h, hbB)
    if dx == 0 then return false end
    wA.obj[0].x = wA.obj[0].x + dx
    if Sprite._retRet and Sprite._retRet(Sprite.hitbox(wA, ox, oy, w, h), hbB) then
        wA.obj[0].x = wA.obj[0].x - dx; return true
    end; return false
end
local function _moverY(dy, wA, ox, oy, w, h, hbB)
    if dy == 0 then return false end
    wA.obj[0].y = wA.obj[0].y + dy
    if Sprite._retRet and Sprite._retRet(Sprite.hitbox(wA, ox, oy, w, h), hbB) then
        wA.obj[0].y = wA.obj[0].y - dy; return true
    end; return false
end

function Sprite.moverComColisao(dx, dy, wA, ox, oy, w, h, hbB)
    return _moverX(dx, wA, ox, oy, w, h, hbB), _moverY(dy, wA, ox, oy, w, h, hbB)
end

function Sprite.resolverColisaoSolida(hbA, hbB, wA)
    local info = Sprite._retRetInfo and Sprite._retRetInfo(hbA, hbB) or { colidindo=false }
    if not info.colidindo then return info end
    wA.obj[0].x = wA.obj[0].x + info.dx; wA.obj[0].y = wA.obj[0].y + info.dy
    return info
end

function Sprite.destruir(w, liberarSprite)
    if w == nil or w._destruido then return end
    w._destruido = true
    local chave = tostring(ffi.cast("void*", w.obj))
    for k in pairs(_animEstado) do if k:find(chave, 1, true) then _animEstado[k] = nil end end
    _espelhos[chave] = nil
    if liberarSprite and w.obj[0].imagem ~= nil then
        C.tupi_sprite_destruir(w.obj[0].imagem); w.obj[0].imagem = nil
    end
end

function Sprite.destruido(w) return w == nil or w._destruido == true end

Visual.sprite = Sprite

local Camera = {}
Camera.camera  = {}
Camera.paralax = {}

local Cam = Camera.camera

function Cam.criar(ax, ay, anc_x, anc_y)
    local ptr = ffi.new("TupiCamera[1]")
    ptr[0] = C.tupi_camera_criar(ax or 0, ay or 0, anc_x or -1, anc_y or -1)
    local obj = { _ptr = ptr }
    local mt = {}; mt.__index = function(t, k) local fn = Cam[k]; if fn then return function(self,...) return fn(self,...) end end end
    setmetatable(obj, mt)
    return obj
end

function Cam.destruir(cam)   assert(cam and cam._ptr); C.tupi_camera_destruir(cam._ptr)          end

function Cam.ativar(cam)     assert(cam and cam._ptr); C.tupi_camera_ativar(cam._ptr)            end
function Cam.pos(cam, x, y)  assert(cam and cam._ptr); C.tupi_camera_pos(cam._ptr, x or 0, y or 0) end
function Cam.mover(cam,dx,dy) assert(cam and cam._ptr); C.tupi_camera_mover(cam._ptr,dx or 0,dy or 0) end
function Cam.zoom(cam, z)    assert(cam and cam._ptr); C.tupi_camera_zoom(cam._ptr, z or 1)      end
function Cam.rotacao(cam, a) assert(cam and cam._ptr); C.tupi_camera_rotacao(cam._ptr, a or 0)   end
function Cam.ancora(cam,ax,ay) assert(cam and cam._ptr); C.tupi_camera_ancora(cam._ptr,ax or -1,ay or -1) end

function Cam.seguir(cam, x, y, vel)
    assert(cam and cam._ptr)
    C.tupi_camera_seguir(cam._ptr, x or 0, y or 0, vel or 0.1, Camera._getDt and Camera._getDt() or 0)
end
function Cam.posAtual(cam)
    return tonumber(C.tupi_camera_get_x(cam._ptr)), tonumber(C.tupi_camera_get_y(cam._ptr))
end
function Cam.alvoAtual(cam)   return tonumber(cam._ptr[0].alvo_x), tonumber(cam._ptr[0].alvo_y) end
function Cam.zoomAtual(cam)   return tonumber(C.tupi_camera_get_zoom(cam._ptr))    end
function Cam.rotacaoAtual(cam) return tonumber(C.tupi_camera_get_rotacao(cam._ptr)) end
function Cam.telaMundo(cam, sx, sy)
    local wx,wy = ffi.new("float[1]"), ffi.new("float[1]")
    C.tupi_camera_tela_mundo_lua(sx, sy, wx, wy); return tonumber(wx[0]), tonumber(wy[0])
end
function Cam.mundoTela(cam, wx, wy)
    local sx,sy = ffi.new("float[1]"), ffi.new("float[1]")
    C.tupi_camera_mundo_tela_lua(wx, wy, sx, sy); return tonumber(sx[0]), tonumber(sy[0])
end
function Cam.mouseMundo(cam)
    local mx, my = Camera._getMouse and Camera._getMouse() or 0, 0
    return Cam.telaMundo(cam, mx, my)
end

local Par = Camera.paralax

function Par.registrar(fatorX, fatorY, z, lLoop, aLoop)
    local id = C.tupi_paralax_registrar(fatorX, fatorY, z or 0, lLoop or 0.0, aLoop or 0.0)
    if id < 0 then error("[Paralax] limite de camadas atingido!") end
    return id
end
function Par.remover(id)     C.tupi_paralax_remover(id)   end
function Par.resetar()       C.tupi_paralax_resetar()      end
function Par.resetarCamada(id) C.tupi_paralax_resetar_camada(id) end
function Par.setFator(id, fx, fy) C.tupi_paralax_set_fator(id, fx or 0, fy or 0) end
function Par.totalAtivas()   return tonumber(C.tupi_paralax_total_ativas()) end

function Par.atualizar(cam, camX, camY)
    if camX == nil and cam and cam._ptr then camX, camY = Cam.posAtual(cam) end
    C.tupi_paralax_atualizar(camX or 0, camY or 0)
end
function Par.offset(id)
    local r = C.tupi_paralax_offset(id)
    assert(r.valido == 1, "[Paralax] ID " .. id .. " inválido")
    return tonumber(r.offset_x), tonumber(r.offset_y), tonumber(r.z_layer)
end

function Par.desenhar(id, wrapper)
    local r = C.tupi_paralax_offset(id)
    if r.valido == 0 then return end
    local ox,oy = tonumber(wrapper.obj[0].x), tonumber(wrapper.obj[0].y)
    wrapper.obj[0].x = ox + tonumber(r.offset_x)
    wrapper.obj[0].y = oy + tonumber(r.offset_y)
    C.tupi_objeto_enviar_batch(wrapper.obj, r.z_layer)
    wrapper.obj[0].x = ox; wrapper.obj[0].y = oy
end

function Par.desenharTile(id, wrapper, largTela)
    local r = C.tupi_paralax_offset(id)
    if r.valido == 0 then return end
    local ox,oy = tonumber(wrapper.obj[0].x), tonumber(wrapper.obj[0].y)
    local offX, offY = tonumber(r.offset_x), tonumber(r.offset_y)
    local larg = tonumber(wrapper.obj[0].largura)
    largTela = largTela or (Camera._getLargura and Camera._getLargura() or 800)
    for i = 0, math.ceil(largTela / larg) + 1 do
        wrapper.obj[0].x = ox + offX + i * larg
        wrapper.obj[0].y = oy + offY
        C.tupi_objeto_enviar_batch(wrapper.obj, r.z_layer)
    end
    wrapper.obj[0].x = ox; wrapper.obj[0].y = oy
end

Visual.camera = Camera

local Texto = {}
local _quad_glifo = ffi.new("float[16]")
local _fonte_padrao = nil

function Texto.carregarFonte(caminho, largCelula, altCelula, colunas, charInicio)
    assert(type(caminho)=="string" and type(largCelula)=="number" and type(altCelula)=="number")
    local spr = C.tupi_sprite_carregar(caminho)
    if spr == nil then error("[Texto] falha ao carregar '" .. caminho .. "'") end
    colunas    = colunas    or 16
    charInicio = charInicio or 32
    local lw, lh = tonumber(spr.largura), tonumber(spr.altura)
    return {
        sprite=spr, textura_id=tonumber(spr.textura),
        larg_cel=largCelula, alt_cel=altCelula,
        colunas=colunas, linhas=math.floor(lh/altCelula),
        larg_img=lw, alt_img=lh,
        uv_px_x = largCelula / lw,uv_px_y = altCelula  / lh,uv_texel_x = 0.5 / lw,uv_texel_y = 0.5 / lh,
        char_ini=charInicio, espaco=largCelula, _destruida=false,
    }
end

function Texto.destruirFonte(fonte)
    if not fonte or fonte._destruida then return end
    C.tupi_sprite_destruir(fonte.sprite); fonte._destruida = true
    if _fonte_padrao == fonte then _fonte_padrao = nil end
end

function Texto.setFontePadrao(fonte) assert(fonte and not fonte._destruida); _fonte_padrao = fonte end
function Texto.getFontePadrao()      return _fonte_padrao end

local function _uv_char(f, codigo)
    local idx = math.max(codigo - f.char_ini, 0)
    if idx >= f.colunas * f.linhas then idx = 0 end
    local col = idx % f.colunas
    local lin = math.floor(idx / f.colunas)

    -- Calcula em pixels absolutos PRIMEIRO
    local px0 = col * f.larg_cel
    local py0 = lin * f.alt_cel
    local px1 = px0 + f.larg_cel
    local py1 = py0 + f.alt_cel

    -- Divide pelo tamanho total da imagem apenas no final
    local u0 = px0 / f.larg_img
    local v0 = py0 / f.alt_img
    local u1 = px1 / f.larg_img
    local v1 = py1 / f.alt_img

    -- Retorne o UV exato, SEM somar ou subtrair texels!
    return u0, v0, u1, v1
end

local _cor_texto = { r=1.0, g=1.0, b=1.0, a=1.0 }

function Texto.setCor(r, g, b, a)
    _cor_texto.r = r or 1.0
    _cor_texto.g = g or 1.0
    _cor_texto.b = b or 1.0
    _cor_texto.a = a or 1.0
end

local function _enviar_glifo(f, codigo, x, y, sw, sh, alpha, z)
    if codigo == 32 then return end
    local u0, v0, u1, v1 = _uv_char(f, codigo)
    
    -- FIX: Arredondar a largura e altura (sw, sh) também, 
    -- igual você fez no Sprite, para evitar desalinhamento de subpixel no Vulkan
    local rx0 = math.floor(x + 0.5)
    local ry0 = math.floor(y + 0.5)
    local rx1 = rx0 + math.floor(sw + 0.5)
    local ry1 = ry0 + math.floor(sh + 0.5)

    _quad_glifo[0]  = rx0; _quad_glifo[1]  = ry0; _quad_glifo[2]  = u0; _quad_glifo[3]  = v0
    _quad_glifo[4]  = rx1; _quad_glifo[5]  = ry0; _quad_glifo[6]  = u1; _quad_glifo[7]  = v0
    _quad_glifo[8]  = rx0; _quad_glifo[9]  = ry1; _quad_glifo[10] = u0; _quad_glifo[11] = v1
    _quad_glifo[12] = rx1; _quad_glifo[13] = ry1; _quad_glifo[14] = u1; _quad_glifo[15] = v1
    
    C.tupi_objeto_enviar_batch_raw(
        f.textura_id, _quad_glifo,
        _cor_texto.r, _cor_texto.g, _cor_texto.b, _cor_texto.a * alpha,
        z
    )
end

function Texto.desenhar(x, y, z, texto, escala, transparencia, fonte)
    fonte = fonte or _fonte_padrao
    assert(fonte and not fonte._destruida, "[Texto] nenhuma fonte definida")
    assert(type(texto) == "string")
    z = z or 0; escala = escala or 1.0; transparencia = transparencia or 1.0

    local sw = fonte.larg_cel * escala
    local sh = fonte.alt_cel  * escala
    local cx = x
    for i = 1, #texto do
        local cod = string.byte(texto, i)
        if cod == 10 then
            cx = x; y = y + sh
        else
            _enviar_glifo(fonte, cod, cx, y, sw, sh, transparencia, z)
            cx = cx + sw
        end
    end
end

function Texto.desenharSombra(x, y, z, dX, dY, texto, escala, transp, escSombra, transpSombra, fonte, fonteSombra)
    fonte = fonte or _fonte_padrao; fonteSombra = fonteSombra or fonte
    assert(fonte and fonteSombra and not fonte._destruida)
    z=z or 0; dX=dX or 1; dY=dY or 1; escala=escala or 1.0; transp=transp or 1.0
    escSombra=escSombra or escala; transpSombra=transpSombra or (transp*0.6)
    local zS = z - 1
    local lw=fonte.larg_cel*escala; local lh=fonte.alt_cel*escala
    local sw=fonteSombra.larg_cel*escSombra; local sh=fonteSombra.alt_cel*escSombra
    for _, dir in ipairs({{dX,dY},{-dX,dY},{dX,-dY},{-dX,-dY}}) do
        local cx,cy = x,y
        for i = 1,#texto do
            local cod=string.byte(texto,i)
            if cod==10 then cx=x;cy=cy+sh else _enviar_glifo(fonteSombra,cod,cx+dir[1],cy+dir[2],sw,sh,transpSombra,zS);cx=cx+sw end
        end
    end
    local cx,cy = x,y
    for i = 1,#texto do
        local cod=string.byte(texto,i)
        if cod==10 then cx=x;cy=cy+lh else _enviar_glifo(fonte,cod,cx,cy,lw,lh,transp,z);cx=cx+lw end
    end
end
Texto.desenharBorda = Texto.desenharSombra

function Texto.desenharCaixa(x, y, z, largura, altura, texto, escala, transp, fonte, frame, tamTile, escBorda, transpBorda, recuo)
    fonte = fonte or _fonte_padrao
    assert(fonte and not fonte._destruida and frame ~= nil and type(texto) == "string")
    z=z or 0; escala=escala or 1.0; transp=transp or 1.0; escBorda=escBorda or 1.0; transpBorda=transpBorda or 1.0
    local tamSrc = tamTile or math.floor(tonumber(frame.largura) / 3)
    local tamDst = tamSrc * escBorda
    recuo = recuo or tamDst
    if not largura or largura==0 then largura = Texto.largura(fonte,texto,escala) + recuo*2 end
    if not altura  or altura ==0 then altura  = Texto.altura(fonte,texto,escala) + recuo*2 end

    largura = math.max(largura, tamDst * 2)
    altura  = math.max(altura,  tamDst * 2)
    local texId=tonumber(frame.textura); local imgW=tonumber(frame.largura); local imgH=tonumber(frame.altura)

    local uvTW = tamSrc / imgW
    local uvTH = tamSrc / imgH
    local texelW = 0.5 / imgW
    local texelH = 0.5 / imgH
    local q = ffi.new("float[16]")
    local function _q(px,py,pw,ph,u0,v0,u1,v1)
        q[0]=px;q[1]=py;q[2]=u0;q[3]=v0; q[4]=px+pw;q[5]=py;q[6]=u1;q[7]=v0
        q[8]=px;q[9]=py+ph;q[10]=u0;q[11]=v1; q[12]=px+pw;q[13]=py+ph;q[14]=u1;q[15]=v1
        C.tupi_objeto_enviar_batch_raw(texId, q, 1.0, 1.0, 1.0, transpBorda, z)
    end
    local function uv(col,lin)
        local u0 = col * uvTW + texelW
        local v0 = lin * uvTH + texelH
        return u0, v0, u0 + uvTW - texelW * 2.0, v0 + uvTH - texelH * 2.0
    end
    local x2,y2,iw,ih = x+largura, y+altura, largura-tamDst*2, altura-tamDst*2
    _q(x,y,tamDst,tamDst,uv(0,0)); _q(x2-tamDst,y,tamDst,tamDst,uv(2,0))
    _q(x,y2-tamDst,tamDst,tamDst,uv(0,2)); _q(x2-tamDst,y2-tamDst,tamDst,tamDst,uv(2,2))
    if iw>0 then _q(x+tamDst,y,iw,tamDst,uv(1,0)); _q(x+tamDst,y2-tamDst,iw,tamDst,uv(1,2)) end
    if ih>0 then _q(x,y+tamDst,tamDst,ih,uv(0,1)); _q(x2-tamDst,y+tamDst,tamDst,ih,uv(2,1)) end
    if iw>0 and ih>0 then _q(x+tamDst,y+tamDst,iw,ih,uv(1,1)) end
    local sw=fonte.larg_cel*escala; local sh=fonte.alt_cel*escala
    local cx,cy = x+recuo, y+recuo
    for i=1,#texto do
        local cod=string.byte(texto,i)
        if cod==10 then cx=x+recuo;cy=cy+sh else _enviar_glifo(fonte,cod,cx,cy,sw,sh,transp,z+1);cx=cx+sw end
    end
end

function Texto.largura(fonte, texto, escala)
    fonte=fonte or _fonte_padrao; escala=escala or 1.0
    local maior,atual = 0,0
    for i=1,#texto do
        if string.byte(texto,i)==10 then if atual>maior then maior=atual end; atual=0
        else atual=atual+1 end
    end
    if atual>maior then maior=atual end
    return maior * fonte.larg_cel * escala
end
function Texto.altura(fonte, texto, escala)
    fonte=fonte or _fonte_padrao; escala=escala or 1.0
    local linhas=1
    for i=1,#texto do if string.byte(texto,i)==10 then linhas=linhas+1 end end
    return linhas * fonte.alt_cel * escala
end
function Texto.dimensoes(fonte, texto, escala)
    return Texto.largura(fonte,texto,escala), Texto.altura(fonte,texto,escala)
end

Visual.texto = Texto

function Visual.patchCor(Tupi)
    if not Tupi or Tupi._texto_cor_patchado then return Tupi end

    local cor_original = Tupi.cor
    local usar_cor_original = Tupi.usarCor

    Tupi.cor = function(r, g, b, a)
        Texto.setCor(r, g, b, a)
        return cor_original(r, g, b, a)
    end

    Tupi.usarCor = function(tc, a)
        local alpha = a or (tc and tc[4]) or 1.0
        if tc then Texto.setCor(tc[1], tc[2], tc[3], alpha) end
        return usar_cor_original(tc, a)
    end

    Tupi._texto_cor_patchado = true
    return Tupi
end

local Fade = {}
Fade.__index = Fade

local COR_R = 0x19/255; local COR_G = 0x14/255; local COR_B = 0x2b/255

function Fade.novo(Tupi, largura, altura, duracao)
    return setmetatable({
        Tupi=Tupi, largura=largura or 160, altura=altura or 144,
        duracao=duracao or 0.4, alpha=0, estado="idle",
        ativo=false, _timer=0, _callback=nil
    }, Fade)
end

function Fade:iniciar(callback, duracao)
    if self.estado ~= "idle" then return end
    self._callback=callback; self._dur=duracao or self.duracao
    self._timer=0; self.alpha=0; self.estado="saindo"; self.ativo=true
end

function Fade:livre() return self.estado == "idle" end

function Fade:atualizar(dt)
    if self.estado == "idle" then return end
    self._timer = self._timer + dt
    if self.estado == "saindo" then
        self.alpha = math.min(self._timer / self._dur, 1)
        if self._timer >= self._dur then
            self.alpha = 1
            if self._callback then self._callback(); self._callback = nil end
            self._timer = 0; self.estado = "entrando"
        end
    elseif self.estado == "entrando" then
        self.alpha = 1 - math.min(self._timer / self._dur, 1)
        if self._timer >= self._dur then self.alpha=0; self.estado="idle"; self.ativo=false end
    end
end

function Fade:desenhar()
    if self.alpha <= 0 then return end
    local T = self.Tupi
    T.cor(COR_R, COR_G, COR_B, self.alpha)
    T.retangulo(-0, -0, T.largura()*2, T.altura()*2)
    T.cor(1, 1, 1, 1)
end

Visual.fade = Fade

return Visual