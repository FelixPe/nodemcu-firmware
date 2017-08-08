# Websocket Module
| Since  | Origin / Contributor  | Maintainer  | Source  |
| :----- | :-------------------- | :---------- | :------ |
| 2017-08-08 | [Felix Penzlin](https://github.com/FelixPe) | [Felix Penzlin](https://github.com/FelixPe) | [xml.c](../../../app/modules/xml.c)|

A module for handling xml based on the tiny parser library [yxml](https://dev.yorhel.nl/yxml) by Yoran Heling. 

The implementation supports the conversion in both direction: A string to a table and a table to a string.

## xml.toTable(xmlstring, startAt)

Converts a a valid xml string into a table with the following structure:
Attributes are accessed by their name as table entries.
The tag name is saved as under the key "xml", which is reserved in xml.
Children are saved under numeric keys starting at 1 as usual.
For example the following string:

<test tag1="v1" tag2="v2">
   <foo>test</foo>
   <bar />
</test>

maps to:

{
["xml"]="test",["tag1"]="v1",["tag2"]="v2",
  [1]=
     {
        ["xml"]="foo",[1]="test"
     },
  [2]=
     {
        ["xml"]="bar"
     }
}


Beside the table the position in the string is returned where the parser stopped. In a valid xml document this will be the last character, but if the given string contained multiple xml-fragments the parser could be called multiple times and each fragment is parsed into a table.

#### Syntax
`xml.toTable(xmlstring,startAt)`

#### Parameters
- `xmlstring` the string to convert.
- `startAt` the position to start at. (default: 0)

#### Returns
`table` xml-structure as table
`integer` the position in the string where the parser stopped

#### Example
```lua
local xmltab, numchar = xml.toTable("<test val='foo'>bar</test>")
print(xmltab["val"]) -- prints "foo"
```

## xml.fromTable(table)

Converts a given table into a xml-fragment.

#### Syntax
`xmlstr = xml.fromTable(table)`

#### Parameters
`table` - table to convert

#### Returns
`string` - xml fragment of the converted table

#### Example
```lua
str = xml.fromTable({["xml"]="test",[1]="bla"}) -- str="<test>bla</test>"

```

