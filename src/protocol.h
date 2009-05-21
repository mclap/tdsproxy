/*
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
-------------------------------------------------
12 01 00 2F 00 00 01 00 00 00 1A 00 06 01 00 20
00 01 02 00 21 00 01 03 00 22 00 04 04 00 26 00
01 FF 09 00 00 00 00 00 01 00 B8 0D 00 00 01

00 04 04 00 26 00 01

PRELOGIN ::= HDR TOKENLIST DATA
TOKENLIST ::= TOKENLAST | TOKEN | TOKENLIST
TOKENLAST ::= 0xFF
TOKEN ::= TYPE(1) POS(2) LENGTH(2)

<TokenType>
  <BYTE>00 </BYTE> - VERSION
</TokenType>
<TokenPosition>
  <USHORT>00 1A</USHORT>
</TokenPosition>
<TokenLeng>
  <USHORT>00 06</USHORT>
</TokenLeng>
<TokenType>
  <BYTE>01 </BYTE> - ENCRYPTION
</TokenType>
<TokenPosition>
  <USHORT>00 20</USHORT>
</TokenPosition>
<TokenLeng>
  <USHORT>00 01</USHORT>
</TokenLeng>
<TokenType>
  <BYTE>02 </BYTE> - INSTVALIDITY
</TokenType>
<TokenPosition>
<USHORT>00 21</USHORT>
</TokenPosition>
<TokenLeng>
<USHORT>00 01</USHORT>
</TokenLeng>
<TokenType>
<BYTE>03 </BYTE> - THREADID
</TokenType>
<TokenPosition>
<USHORT>00 22</USHORT>
</TokenPosition>
<TokenLeng>
<USHORT>00 04</USHORT>
</TokenLeng>
<TokenType>
<BYTE>04 </BYTE> - MARS
</TokenType>
<TokenPosition>
<USHORT>00 26</USHORT>
</TokenPosition>
<TokenLeng>
<USHORT>00 01</USHORT>
</TokenLeng>
<TokenType>
<BYTE>FF </BYTE> - TOKENLAST
</TokenType>

<PreloginData>
<BYTES>09 00 00 00 00 00 01 00 B8 0D 00 00 01</BYTE>
</PreloginData>

*/

struct pkt_header
{
	uint8_t type;
	uint8_t eof;
	/** size of whole packet (header + data) net-order */
	uint16_t length; // net-order
	uint16_t spid; // net-order
	uint16_t packet; /* sequence number ?, net-order */
	uint8_t window;
};

/*
struct pkt_data_prelogin
{
	uint8_t tok_type;
	uint16_t tok_pos;
	uint16_t tok_leng;

struct pkt_prelogin
{
	struct pkt_header hdr;
	struct pkt_data data;
*/
