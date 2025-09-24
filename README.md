Boa! 🚀 Então vamos montar um **roadmap detalhado** de tudo que precisa ser construído para o desenvolvimento de um emulador de NES. A ideia é ir em camadas, construindo do mais simples ao mais complexo, até chegar em algo que rode *Super Mario Bros* ou *Mega Man*.  

---

### 🛠️ Roadmap de um Emulador de NES

#### **Etapa 1 – Fundamentos**
1. **Loader de ROMs**
   - Ler arquivo `.nes` (iNES format).
   - Extrair **header**, PRG-ROM (programa) e CHR-ROM (gráficos).
   - Identificar **mapper**, mirroring, tamanho de bancos.

2. **Estrutura de Memória**
   - Implementar **64KB address space da CPU**:
     - RAM (0x0000 – 0x07FF).
     - Registros da PPU (0x2000 – 0x2007).
     - Registros da APU e Input (0x4000 – 0x4017).
     - Espaço PRG-ROM do cartucho (a partir de 0x8000).

---

#### **Etapa 2 – CPU (Ricoh 2A03 / 6502 compatível)**
3. **Emular registradores da CPU**
   - PC (Program Counter), SP (Stack Pointer), A (Acumulador), X, Y, P (Status).
4. **Emular instruções**
   - Todas as 6502 standard (sem BCD).
   - Classes: Load/Store, Arithmetic, Branch, Jumps, Bitwise, Stack Ops.
5. **Timers e ciclos**
   - Emular instruções com ciclo correto (determinante para sincronizar PPU/APU).
6. **Stack & Interrupções**
   - IRQ, NMI, Reset (muito usados pelo PPU e jogos).

---

#### **Etapa 3 – Mappers (cartuchos)**
7. **Mapper NROM (Mapper 0)**
   - O mais simples — usado por *Donkey Kong*, *Super Mario Bros 1*.
8. **Mapper MMC1, MMC3 (os mais comuns)**
   - Troca de bancos PRG/CHR.
   - Funcionalidade de IRQs (para efeitos gráficos avançados).
9. **Outros mappers populares** conforme necessidade.

---

#### **Etapa 4 – PPU (gráficos)**
10. **Nome tables & Pattern tables**
    - Renderizar background tiles (CHR-ROM ou CHR-RAM).
11. **Sprites/OAM**
    - Renderizar sprites 8x8 ou 8x16.
    - Suporte a prioridades (BG ↔ Sprite), colisões.
12. **Paleta de cores**
    - Implementar a paleta oficial (54 cores).
13. **Emulação de ciclos da PPU (preciso!)**
    - 1 linha de 341 ciclos.
    - 262 linhas por quadro (frame).
    - Sincronizar CPU + PPU (nesse ponto temos vídeo funcional).

---

#### **Etapa 5 – APU (som)**
14. **Canais básicos**
    - Square Wave 1/2
    - Triangle Wave
    - Noise
    - DPCM
15. **Mixagem/interpolação**
    - Ajustar output de áudio na taxa correta (NTSC: ~60Hz → 44100Hz).
16. **Sincronização com CPU**
    - APU avança junto com a execução de instruções.

---

#### **Etapa 6 – Entrada (controles)**
17. **Input (0x4016 / 0x4017 registers)**
    - Emulação da leitura serial dos botões.
    - Mapear teclado/joystick para NesPad.

---

#### **Etapa 7 – Camada de integração**
18. **Timing**
    - Para cada ciclo da CPU, avançar 3 ciclos da PPU.
    - Atualizar APU junto.
19. **Loop principal**
    - Executa ciclo da CPU.
    - Atualiza PPU.
    - Atualiza APU.
    - Renderiza quadro quando necessário.
20. **Depuração**
    - Criar modo de *debugger* (dump memória, estado CPU, step by step).

---

#### **Etapa 8 – Polimento**
21. **Melhorar compatibilidade**
    - Suporte a mappers adicionais.
    - Correção de bordas, raster effects.
22. **UI básica**
    - Janela para rodar jogos.
    - Botões de reset/save state/load state.
23. **Performance**
    - Otimizar laços críticos.
    - Sincronizar emulador com tempo real (60 fps).

---

### 🏁 Resultado esperado
- Depois da **Etapa 3 + Etapa 4 básica**, já é possível rodar *Super Mario Bros* sem som.  
- Depois da **Etapa 5**, teremos áudio funcional.  
- As etapas finais dão compatibilidade e refinamentos.

---

👉 Minha sugestão: começamos **pela CPU** (já que tudo depende dela).  
Quer que eu detalhe **como iniciar a CPU 6502 (registradores, instruções e ciclo de fetch-decode-execute)** como primeiro passo prático?