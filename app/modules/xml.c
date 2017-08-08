#include "lmem.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "module.h"

#include "yxml.h"

#define XML_BUFSIZE 2048
#define CONTXML_BUFSIZE 2048
#define ATTRXML_BUFSIZE 128

static const char* parsing_error_names[] = {"EOF", "invalid character", "wrong tag", "stack overflow", "unexpected byte"};

void static table2xmlbuffer(lua_State *L, luaL_Buffer *b){
    luaL_putchar(b, '<');
    lua_pushstring(L, "xml");
    lua_rawget(L,-2);
    if(lua_isnil(L, -1))
    {
        luaL_error(L, "no tag");
    }
    luaL_addvalue(b);

    lua_pushnil(L);
    while(lua_next(L, -2) != 0)
    {
         if(lua_istable(L, -1))
         {
             lua_pop(L, 1);
         }
         else if(lua_isnumber(L, -2))
         {
             lua_pop(L, 1);
         }
         else
         {
    	    const char *key = lua_tostring(L, -2);
	    if(key[0] == 'x' && key[1] == 'm' && key[2] == 'l' && key[3] == '\0')
            {
                lua_pop(L, 1);
            }
            else
            {
                
                luaL_putchar(b, ' ');
                luaL_addstring(b, key);
                luaL_addlstring(b, "=\"", 2);
                luaL_addvalue(b);
                luaL_putchar(b, '"');
            }
         }
    }

    int i = 1;
    lua_rawgeti(L, -1, i);
    if(!lua_isnil(L, -1))
    {
       luaL_putchar(b, '>');
    }

    while( !lua_isnil(L, -1))
    {
        if(lua_istable(L, -1))
        {   
            table2xmlbuffer(L, b);
            lua_pop(L, 1);
        }
        else if(lua_isstring(L, -1))
        {
            luaL_addvalue(b);
        }
        else lua_pop(L, 1);
	i++;
        lua_rawgeti(L, -1, i);
    }
    lua_pop(L, 1);
       
    
    if(i > 1)
    {
       luaL_putchar(b, '<');
       luaL_putchar(b, '/');
       lua_pushstring(L, "xml");
       lua_rawget(L,-2);
       luaL_addvalue(b);
   }
   else
   {
       luaL_putchar(b, ' ');
       luaL_putchar(b, '/');
   }   
   luaL_putchar(b, '>');
}

static int table2xml(lua_State *L){
   const int number_of_arguments = lua_gettop(L);
   int depth = 0;

   if(number_of_arguments != 1)
   {
       luaL_error(L, "num args");
   }

   luaL_checktype(L, 1, LUA_TTABLE);

   luaL_Buffer b; // implement your own buffer, this one needs the lua stack!
   luaL_buffinit( L, &b );
   char *p = luaL_prepbuffer(&b);    
     
   table2xmlbuffer(L, &b);
  
   luaL_pushresult( &b );
   return 1;
}

static int isWhitespace(const char c){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int xml2table(lua_State *L){
   const int number_of_arguments = lua_gettop(L);
   const char* xmlstr;
   int n_start = 0;
   char attrbuf[ATTRXML_BUFSIZE]; 
   char contbuf[CONTXML_BUFSIZE];   
   char* attrbufptr;
   char* contbufptr;
   int initialTop = lua_gettop(L);
   yxml_t xml;
   yxml_ret_t r;

   switch(number_of_arguments){
        case 1:
        xmlstr = luaL_checkstring(L, -1);
        break;
        case 2:
        n_start = luaL_checkinteger(L, -1);
   	xmlstr = luaL_checkstring(L, -2);
        break;
	default:
        luaL_error(L, "num args");
   }

   char buf[XML_BUFSIZE];
   yxml_init(&xml, buf, XML_BUFSIZE);
  
   attrbufptr = attrbuf;
   contbufptr = contbuf;
   
   for(xmlstr+=n_start; *xmlstr; xmlstr++){
      r = yxml_parse(&xml, *xmlstr);

      if(r < 0)
      {
         luaL_error(L, "xml error: %s", parsing_error_names[r+5]);
         return 0;
      }
      else switch(r)
      {
         case YXML_ATTRSTART:
            attrbufptr = attrbuf;
            break;
         case YXML_ATTRVAL:
            if(attrbufptr-attrbuf > ATTRXML_BUFSIZE)
	    {
	       luaL_error(L, "buf size");
	    }
            *(attrbufptr++) = xml.data[0];
            break;
          case YXML_ATTREND:
            *attrbufptr = 0;
            lua_pushstring(L, xml.attr); /*push key*/
            lua_pushlstring(L, attrbuf, attrbufptr-attrbuf); /*push value*/
            lua_rawset(L, -3);
            break;
          case YXML_ELEMSTART:
	    if(contbufptr > contbuf)
            {
		while(isWhitespace(*(contbufptr-1))){
                   contbufptr--;
                }
                *contbufptr = 0;

	        lua_pushinteger(L, lua_objlen(L,-1)+1);
                lua_pushlstring(L, contbuf, contbufptr-contbuf); /*push value*/
                contbufptr = contbuf;
                //lua_rawseti (L, -2, lua_objlen(L,-2)+1);
                lua_settable(L, -3);
            }
            if(lua_gettop(L) > initialTop)
               lua_pushinteger(L, lua_objlen(L,-1)+1);
	    lua_newtable(L);
            lua_pushlstring(L, "xml", 3); /*push key*/
            lua_pushlstring(L, xml.elem, yxml_symlen(&xml, xml.elem)); /*push value*/
            lua_rawset(L, -3);
            break;
          case YXML_ELEMEND:
	    if(contbufptr > contbuf)
            {

		while(isWhitespace(*(contbufptr-1))){
                   contbufptr--;
                }
                *contbufptr = 0;

                lua_pushlstring(L, contbuf, contbufptr-contbuf); /*push value*/
                contbufptr = contbuf;
                lua_rawseti (L, -2, lua_objlen(L,-2)+1);
            }

            if(lua_gettop(L) > initialTop+1){
                lua_rawset(L, -3);
            }
            else
            {
		int i = 1;
                while( isWhitespace(*(xmlstr+i)) ){
                   i++;
		}
		lua_pushinteger(L, xml.total+i-1+n_start);
		return 2;
            }
            break;
          case YXML_CONTENT:
            if(contbufptr-contbuf > CONTXML_BUFSIZE)
	    {
	       luaL_error(L, "buf size");
	    }
	    if(contbufptr != contbuf || // nur front trimmen
		!isWhitespace(xml.data[0]))
            {
                *(contbufptr++) = xml.data[0];
            }
            break;
       }
   }    

	
   r=yxml_eof(&xml);
   if(r < 0)
   {
       luaL_error(L, "xml error: %s", parsing_error_names[r+5]);
   }

   lua_pushinteger(L, xml.total);   
   return 2;
}

static const LUA_REG_TYPE xml_map[] =
{
  { LSTRKEY("toTable"), LFUNCVAL(xml2table)},
  { LSTRKEY("fromTable"), LFUNCVAL(table2xml)},
  { LNILKEY, LNILVAL }
};

/*
static const LUA_REG_TYPE websocketclient_map[] =
{
  { LSTRKEY("on"), LFUNCVAL(websocketclient_on) },
  { LSTRKEY("connect"), LFUNCVAL(websocketclient_connect) },
  { LSTRKEY("send"), LFUNCVAL(websocketclient_send) },
  { LSTRKEY("close"), LFUNCVAL(websocketclient_close) },
  { LSTRKEY("__gc" ), LFUNCVAL(websocketclient_gc) },
  { LSTRKEY("__index"), LROVAL(websocketclient_map) },
  { LNILKEY, LNILVAL }
};
*/

int loadXmlModule(lua_State *L) {
  // luaL_rometatable(L, METATABLE_XML, (void *) websocketclient_map);

  return 0;
}

NODEMCU_MODULE(XML, "xml", xml_map, loadXmlModule);


