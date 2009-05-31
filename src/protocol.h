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

struct tds_header
{
	uint8_t type;
	uint8_t eof;
	/** size of whole packet (header + data) net-order */
	uint16_t length; // net-order
	uint16_t spid; // net-order
	uint8_t packet_num; /* sequence number ?, net-order */
	uint8_t window;
};

enum {
	pkt_LOGIN7 = 0x10,
	pkt_RESPONSE = 0x04,
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

/* LOGIN7
     <PacketData>
       <Login7>
	  uint32_t length;
	  uint32_t tds_version;
	  uint32_t pkt_size;
	  uint32_t client_version;
	  uint32_t client_pid;
	  uint32_t connection_id;
          <OptionFlags1>
            <BYTE>E0 </BYTE>
          </OptionFlags1>
          <OptionFlags2>
            <BYTE>03 </BYTE>
          </OptionFlags2>
          <TypeFlags>
            <BYTE>00 </BYTE>
          </TypeFlags>
          <OptionFlags3>
            <BYTE>00 </BYTE>
          </OptionFlags3>
          <ClientTimZone>
            <LONG>E0 01 00 00 </LONG>
          </ClientTimZone>
          <ClientLCID>
            <DWORD>09 04 00 00 </DWORD>
          </ClientLCID>
          <OffsetLength>
            <ibHostName>
              <USHORT>5E 00 </USHORT>
            </ibHostName>
            <cchHostName>
              <USHORT>08 00 </USHORT>
            </cchHostName>
            <ibUserName>
              <USHORT>6E 00 </USHORT>
            </ibUserName>
            <cchUserName>
              <USHORT>02 00 </USHORT>
            </cchUserName>
            <ibPassword>
              <USHORT>72 00 </USHORT>
            </ibPassword>
            <cchPassword>
              <USHORT>00 00 </USHORT>
            </cchPassword>
            <ibAppName>
              <USHORT>72 00 </USHORT>
            </ibAppName>
            <cchAppName>
              <USHORT>07 00 </USHORT>
            </cchAppName>
            <ibServerName>
              <USHORT>80 00 </USHORT>
            </ibServerName>
            <cchServerName>
              <USHORT>00 00 </USHORT>
            </cchServerName>
            <ibUnused>
              <USHORT>80 00 </USHORT>
            </ibUnused>
            <cbUnused>
              <USHORT>00 00 </USHORT>
            </cbUnused>
            <ibCltIntName>
              <USHORT>80 00 </USHORT>
            </ibCltIntName>
            <cchCltIntName>
              <USHORT>04 00 </USHORT>
            </cchCltIntName>
            <ibLanguage>
              <USHORT>88 00 </USHORT>
            </ibLanguage>
            <cchLanguage>
              <USHORT>00 00 </USHORT>
            </cchLanguage>
            <ibDatabase>
              <USHORT>88 00 </USHORT>
            </ibDatabase>
            <cchDatabase>
              <USHORT>00 00 </USHORT>
            </cchDatabase>
            <ClientID>
              <BYTES>00 50 8B E2 B7 8F </BYTES>
            </ClientID>
            <ibSSPI>
              <USHORT>88 00 </USHORT>
            </ibSSPI>
            <cbSSPI>
              <USHORT>00 00 </USHORT>
            </cbSSPI>
            <ibAtchDBFile>
          <USHORT>88 00 </USHORT>
        </ibAtchDBFile>
        <cchAtchDBFile>
          <USHORT>00 00 </USHORT>
        </cchAtchDBFile>
        <ibChangePassword>
          <USHORT>88 00 </USHORT>
        </ibChangePassword>
        <cchChangePassword>
          <USHORT>00 00 </USHORT>
        </cchChangePassword>
        <cbSSPILong>
          <LONG>00 00 00 00 </LONG>
        </cbSSPILong>
      </OffsetLength>
      <Data>
        <BYTES>73 00 6B 00 6F 00 73 00 74 00 6F 00 76 00 31 00 73 00 61 00
4F 00 53 00 51 00 4C 00 2D 00 33 00 32 00 4F 00 44 00 42 00 43 00 </BYTES>
      </Data>
    </Login7>
  </PacketData>
*/
