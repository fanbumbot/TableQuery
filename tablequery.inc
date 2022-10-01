/*

	Скрипт реализован на плагине GVar от Incognito.

	Данный инклюд создан для многократного облегчения жизни обывателям и даже умелым скриптерам
	при работы с базами данных (SQLite и MySQL).

	Большая часть работы по созданию, удалению, связыванию данных, поиску нужных значений в
	инклюде переводится в русло данного скрипта, что облегачает жизнь тому, кто пишет сам код.
	
	Работа скрипта основана на абстрагировании от обычной отправки запроса (или заданию его по шаблону)
	и на создании более общих моделей для работы. Так, например, две таблицы с одинаковыми по качеству данными (н-р столбцами)
	строятся на основе одного образа (шаблона) TableImage. Однако таблицы могут быть в одной базе дынных, но с разными
	названиями или вообще в разных базах данных. Это решает проблему создания одинаковых таблиц для разных
	задач, либо при условии, когда определённая таблица не была найдена, но создать подобную ей необходимо для
	продолжения работы (скрипта или самого разработчика).
	
	Таким образом у нас есть шаблон TableImage, который содержит параметры (условно колонки), их размер и тип.
	Из TableImage строятся таблицы Table, у которых есть исходный шаблон TableImage, база данных, в которой она находится,
	а также название таблицы. Уже в таблицу заносятся данные, которые автоматически (если писать их через скрипт,
	а не самому) будут подключены к системе кэшей - к тому, через что можно далее обращаться к данной записи.
	Кэщ содержит номер таблицы Table, в которой находится запись, а также ID записи (который задаётся как
	флаг AUTOINCREMENT в SQLite). Также можно подключить/отключить запись к/от кэшу(-а).
	
	Если таблица SQLite будет изменена извне (добавлена/удалена таблица/колонка/запись), то скрипт не узнает об этом.
	Поэтому, если Вы хотите использовать данный скрипт, то рекомендую Вам по минимуму использовать возможности SQLite,
	или MySQL аналоги которых уже существуют в скрипте, если Вы, конечно, не уверены на 100% в своём решении.
	

	Структуры данных расположенные через GVar

	Пример: result = TableImage_value(num, t#1, a#1, {s#1}, t#2, a#2, {s#2}, ...):
	    _:value - номер образа
	    
	Пояснение:
		result - параметр, который вернёт функция интерфейса
		TableImage - префикс названия GVar'a
		value - номер, который обычно дублируется в result
		(само название GVar'a обычно имеет вид "TableImage_1", "TableImage_2", "TableImage_3" и т.д.)
		
		(num, t#1, a#1, {s#1}, t#2, a#2, {s#2}, ...) - параметры, которые содержит уже сам GVar по внутреннему id
		... - неопределённое количество параметров
		#1, #2 - номера, которые обобщают параметры в объекты
			(в данном примере под одним номер скрывается один спец. параметр)
		{s#1}, {s#2} - необязательные параметры, которые могут отсутствовать при определённых условиях
		
		"_:value - номер образа" - объяснение
		[] - массив неопределённой длины
		[s] - массив длины s
		_: - тэг, используемый для передачи (получения)
		    _ - тэг целого числа
		    string: - имеется в виду _:a[], нужно всего лишь для разницы между строкой и массивом

		
		
	Структуры:

	result = TableImage_value(num, t#1, a#1, {s1}, t#2, a#2, {s#2}, ...):
	    _:value - номер образа
	    _:num - количество параметров
	    _:t - тип параметра (INTEGER, FLOAT или STRING, соотв. 1, 2, 3)
	    string:a - название параметра
	    _:s - размер (если параметр не является строкой или массивом, то не пишется)
	    TableImage:result - номер таблицы (INVALID_TABLEIMAGE если образ таблицы не был создан)
	    
	result = Table_value(tableimage, database, name, tabletype):
	    _:value - номер таблицы
	    TableImage:tableimage - номер образа, на котором будет строится таблица
	    TableDB:database - база данных, в которой будет лежать эта таблица
	    string:name - название таблицы, которое будет использоваться
	    Table:result - номер таблицы (INVALID_TABLE если таблица не была создана)
	    
	result = TableCache_value(table, id):
	    _:value - номер кэша
	    Table:table - номер таблицы
		string:id - ID, используемый в самой таблице для идентификации опр. записи (создаётся БД через AUTOINCREMENT)
	    TableCache:result - номер кэша таблицы (INVALID_TABLECACHE если кэш не был создан)
	    
*/

#if defined tables_included
	#endinput
#endif
#define tables_included

#define TVAR_TYPE_NONE 0
#define TVAR_TYPE_INT 1
#define TVAR_TYPE_FLOAT 2
#define TVAR_TYPE_STRING 3
#define TVAR_TYPE_ARRAY 4

#define INVALID_TABLEIMAGE (TableImage:0)
#define INVALID_TABLE (Table:0)
#define INVALID_TABLECACHE (TableCache:0)

#define TABLEIMAGE_TEMP "TableImage_%i"
#define TABLE_TEMP "Table_%i"
#define TABLECACHE_TEMP "TableCache_%i"
	
#define TableRD<%0,%1,%2> \
    stock %0:operator=(%1:oper) {return %0:((_:oper & 0xFFFF) | (%2 << 16));} \
    stock %1:operator=(%0:oper) {return %1:(_:oper & 0xFFFF);}
    
#define INVALID_TABLEDB (TableDB:0)
#define INVALID_TABLERESULT (TableResult:0)

new static tableimage_count;
new static table_count;
new static tablecache_count;

enum TableImage(){}
enum Table(){}
enum TableCache(){}

enum TableDB(){}
enum TableResult(){}

new static tvar_temp[256];

//Базы данных и результаты

stock _:operator-(TableDB:oper)			return (_:oper & 0xFFFF);
stock _:operator-(TableResult:oper)		return (_:oper & 0xFFFF);

//Привязка баз данных
#if defined _sampdb_included
TableRD<TableDB, DB, 1>
#endif

#if defined mysql_included
TableRD<TableDB, MySQL, 2>
#endif

//Привязка результатов
#if defined _sampdb_included
TableRD<TableResult, DBResult, 1>
#endif

#if defined mysql_included
TableRD<TableResult, Cache, 2>
#endif

stock TableRDUnpack({TableDB, TableResult}:rd, &type = 0, &value = 0)
{
	type = (rd >> 16) & 0xFFFF;
	value = -rd;
	return 1;
}

stock IsValidTableResult(TableResult:result)
{
	new type, value;
	TableRDUnpack(result, type, value);
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return value == 0 ? 0 : 1;
#endif

#if defined mysql_included
		case 2:		return value == 0 ? 0 : 1;
#endif
	}
	return 0;
}

stock TableResultClear(TableResult:result)
{
	new type, value;
	TableRDUnpack(result, type, value);
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_free_result(result);
#endif

#if defined mysql_included
		case 2:		return cache_delete(result);
#endif
	}
	return 0;
}

//Привязка функций работы с базами данных к общим

stock TableResult:TableQuery(TableDB:database, query[], {bool, _}:autodelete)
{
	new type, TableDB:value;
	TableRDUnpack(database, type, _:value);

	new TableResult:result;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		result = db_query(value, query);
#endif

#if defined mysql_included
		case 2:		result = mysql_query(value, query, !autodelete);
#endif
		default:
		{
		    return INVALID_TABLERESULT;
		}
	}
	if(autodelete)
	{
	    TableResultClear(result);
	}
	return result;
}

stock TableNumRows(TableResult:result)
{
	new type, value;
	TableRDUnpack(result, type, value);

	new ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		ret = db_num_rows(result);
#endif

#if defined mysql_included
	    case 2:
		{
		    cache_set_active(result);
			ret = cache_num_rows();
		}
#endif
		default:
		{
		    return 0;
		}
	}
	return ret;
}

stock TableGetFieldAssoc(TableResult:result, field[], dest[], dest_s = sizeof(dest))
{
	new type, value;
	TableRDUnpack(result, type, value);

	new ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		ret = db_get_field_assoc(DBResult:value, field, dest, dest_s);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			ret = cache_get_value_name(0, field, dest, dest_s);
	    }
#endif
		default:
		{
		    return 0;
		}
	}
	return ret;
}

stock TableGetFieldAssocInt(TableResult:result, field[])
{
	new type, value;
	TableRDUnpack(result, type, value);

	new ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_get_field_assoc_int(DBResult:value, field);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			cache_get_value_name_int(0, field, ret);
			return ret;
	    }
#endif
	}
	return 0;
}

stock Float:TableGetFieldAssocFloat(TableResult:result, field[])
{
	new type, value;
	TableRDUnpack(result, type, value);

	new Float:ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_get_field_assoc_float(DBResult:value, field);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			cache_get_value_name_float(0, field, ret);
			return ret;
	    }
#endif
	}
	return 0;
}

stock TableGetField(TableResult:result, id, dest[], dest_s = sizeof(dest))
{
	new type, value;
	TableRDUnpack(result, type, value);

	new ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_get_field(DBResult:value, id, dest, dest_s);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			ret = cache_get_value_index(0, id, dest, dest_s);
			return ret;
	    }
#endif
	}
	return 0;
}

stock TableGetFieldInt(TableResult:result, id)
{
	new type, value;
	TableRDUnpack(result, type, value);

	new ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_get_field_int(DBResult:value, id);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			cache_get_value_index_int(0, id, ret);
			return ret;
	    }
#endif
	}
	return 0;
}

stock Float:TableGetFieldFloat(TableResult:result, id)
{
	new type, value;
	TableRDUnpack(result, type, value);

	new Float:ret;
	switch(type)
	{
#if defined _sampdb_included
	    case 1:		return db_get_field_float(DBResult:value, id);
#endif

#if defined mysql_included
	    case 2:
	    {
		    cache_set_active(Cache:value);
			cache_get_value_index_float(0, id, ret);
			return ret;
	    }
#endif
	}
	return Float:0;
}

//Первичный интерфейс
stock SetGVarTableImageTemp({TableImage, _}:tableimage)
{
	return format(tvar_temp, sizeof(tvar_temp), TABLEIMAGE_TEMP, _:tableimage);
}
stock SetGVarTableTemp({Table, _}:table)
{
	return format(tvar_temp, sizeof(tvar_temp), TABLE_TEMP, _:table);
}
stock SetGVarTableCacheTemp({TableCache, _}:cache)
{
	return format(tvar_temp, sizeof(tvar_temp), TABLECACHE_TEMP, _:cache);
}
stock CheckGVarTemp()
{
    return (GetGVarType(tvar_temp) == GLOBAL_VARTYPE_NONE) ? 0 : 1;
}
stock DeleteGVarTemp(size = 1)
{
    new temp = DeleteGVar(tvar_temp);
	for(new i = 1; i < size; i++)
	{
		DeleteGVar(tvar_temp, i);
	}
	return temp;
}

stock SetGVarIntTemp(value, id)				return SetGVarInt(tvar_temp, value, id);
stock SetGVarFloatTemp(Float:value, id)		return SetGVarFloat(tvar_temp, value, id);
stock SetGVarStringTemp(value[], id)		return SetGVarString(tvar_temp, value, id);

stock GetGVarIntTemp(id)					return GetGVarInt(tvar_temp, id);
stock GetGVarFloatTemp(id)					return GetGVarFloat(tvar_temp, id);
stock GetGVarStringTemp(value[], maxlenght = sizeof(value), id)		return GetGVarString(tvar_temp, value, maxlenght, id);

//Вторичный интерфейс
stock SaveVarTable({Table, _}:table, TableImage:tableimage, TableDB:database, name[])
{
	SetGVarTableTemp(table);
	SetGVarIntTemp(_:tableimage, 0);
	SetGVarIntTemp(_:database, 1);
	SetGVarStringTemp(name, 2);
	return 1;
}

stock LoadVarTable({Table, _}:table, &TableImage:tableimage, &TableDB:database, name[], name_s = sizeof(name))
{
    SetGVarTableTemp(table);
    if(!CheckGVarTemp())    return 0;
	tableimage = TableImage:GetGVarIntTemp(0);
	database = DB:GetGVarIntTemp(1);
	GetGVarStringTemp(name, name_s, 2);
	return 1;
}

stock SaveVarTableCache({TableCache, _}:cache, Table:table, id[])
{
	SetGVarTableCacheTemp(cache);
	SetGVarIntTemp(_:table, 0);
	SetGVarStringTemp(id, 1);
	return 1;
}

stock LoadVarTableCache({TableCache, _}:cache, &Table:table, id[], id_s = sizeof(id))
{
	SetGVarTableCacheTemp(cache);
	if(!CheckGVarTemp())    return 0;
	table = Table:GetGVarIntTemp(0);
    GetGVarStringTemp(id, id_s, 1);
	return 1;
}

//Третичный интерфейс
stock IsValidTableImage(TableImage:tableimage)
{
	if(tableimage == INVALID_TABLEIMAGE)    return 0;
	SetGVarTableImageTemp(tableimage);
	return CheckGVarTemp();
}

stock IsValidTable(Table:table)
{
	if(table == INVALID_TABLE)  return 0;
	SetGVarTableTemp(table);
	return CheckGVarTemp();
}

stock IsValidTableCache(TableCache:cache)
{
	if(cache == INVALID_TABLECACHE) return 0;
	SetGVarTableCacheTemp(cache);
	return CheckGVarTemp();
}

//Четвертичный интерфейс (все основные функции)
//Кэш
stock TableCache:CreateTableCache()
{
	if(TableCache:(tablecache_count+1) == INVALID_TABLECACHE)  return INVALID_TABLECACHE;
    tablecache_count++;
    SaveVarTableCache(tablecache_count, Table:-1, "");
    return TableCache:tablecache_count;
}

stock DestroyTableCache(TableCache:cache)
{
	if(!IsValidTableCache(cache))    return 0;
	SetGVarTableCacheTemp(cache);
	DeleteGVarTemp(2);
	return 1;
}

stock IsTableCacheLinked(TableCache:cache)
{
    if(!IsValidTableCache(cache))    return 0;
    SetGVarTableCacheTemp(cache);
    return IsValidTable(GetGVarInt(tvar_temp));
}

stock LinkTableCache(TableCache:cache, Table:table, id[])
{
	if(!IsValidTableCache(cache))    return 0;
	if(!IsValidTable(table))	return 0;
	
	static tname[256];
	new TableImage:tableimage, TableDB:database;
 	LoadVarTable(table, tableimage, database, tname);
 	
    static str_result[64];
    static expr[256];
    format(expr, sizeof(expr), "SELECT * FROM %s WHERE id = '%s'", tname, id);
	new TableResult:result;
	
	result = TableQuery(database, expr, false);
	new num = TableNumRows(result);
	if(!num)
	{
	    TableResultClear(result);
	    return 0;
	}
	TableGetFieldAssoc(result, "id", str_result);
	TableResultClear(result);
	
	SaveVarTableCache(cache, table, str_result);
	return 1;
}

stock UnLinkTableCache(TableCache:cache)
{
	if(!IsValidTableCache(cache))    return 0;
	if(!IsTableCacheLinked(cache))  return 0;
	SaveVarTableCache(cache, INVALID_TABLECACHE, "");
	return 1;
}

//Образ таблицы

stock TableImage:TableImageCreate({_, Float, bool}:...)
{
    if(TableImage:(tableimage_count+1) == INVALID_TABLEIMAGE)  return INVALID_TABLEIMAGE;
    tableimage_count++;
	
	SetGVarTableImageTemp(tableimage_count);

	new type;
	new count = 0;
    new temp;
	
	InitArgumentMarker(0);
	new len = GetArgumentNum();
	if(len != len)
	{//Избавление от некомпиляции
	    SetGVarString(tvar_temp, "", 0);
	}
	for(new i = 0; i < len; i++)
	{
	    type = GetArgumentRef(i);
        SetGVarInt(tvar_temp, type, i+1);
	    i++;

        temp = GetArgument(i);
        #emit LOAD.S.pri i
        #emit INC.pri
        #emit PUSH.pri
        #emit PUSH.S temp
        #emit PUSH.C tvar_temp
        #emit PUSH.C 12
        #emit SYSREQ.C SetGVarString
        #emit STACK 16

		if(type == TVAR_TYPE_STRING || type == TVAR_TYPE_ARRAY)
		{
		    i++;
		    SetGVarInt(tvar_temp, GetArgumentRef(i), i+1);
		}
		count++;
	}
	SetGVarInt(tvar_temp, count, 0);
	return TableImage:tableimage_count;
}

stock TableImage:GetTableImageByTable(Table:table)
{
	if(!IsValidTable(table))	return INVALID_TABLEIMAGE;
    static name[32];
    format(name, sizeof(name), "Table_%i", _:table);
    return TableImage:GetGVarInt(name, 0);
}

stock GetTableName(Table:table, tablename[], tablename_s = sizeof(tablename))
{
	if(!IsValidTable(table))	return 0;
    static name[32];
    format(name, sizeof(name), "Table_%i", _:table);
    GetGVarString(name, tablename, tablename_s, 2);
    return 1;
}

//Таблицы

stock TableCreateBuild(TableImage:tableimage, tname[], TableDB:database, dest[], dest_s = sizeof(dest))
{
	if(!IsValidTableImage(tableimage))  return 0;
	if(!strlen(tname))  return 0;
	
	static string[256];
	
	strdel(dest, 0, dest_s);
	if((database >> 16) & 0xFFFF)
	{
		format(dest, dest_s, "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT,", tname);
	}
	else
	{
		format(dest, dest_s, "CREATE TABLE %s (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,", tname);
	}
	
	SetGVarTableImageTemp(tableimage);

	new len = GetGVarInt(tvar_temp, 0);
	
	new curarg = 1;
	new type;
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);
	    GetGVarString(tvar_temp, string, sizeof(string), curarg+1);
		switch(type)
		{
		    case TVAR_TYPE_INT:     format(dest, dest_s, "%s%s INTEGER,", dest, string);
		    case TVAR_TYPE_FLOAT:   format(dest, dest_s, "%s%s FLOAT,", dest, string);
		    case TVAR_TYPE_STRING:
			{
				format(dest, dest_s, "%s%s VARCHAR(%i),", dest, string, GetGVarInt(tvar_temp, curarg+2));
				curarg++;
			}
		    case TVAR_TYPE_ARRAY:
			{
				format(dest, dest_s, "%s%s VARCHAR(%i),", dest, string, GetGVarInt(tvar_temp, curarg+2));
				curarg++;
			}
		}
		curarg += 2;
	}
	dest[strlen(dest)-1] = ')';
	if(strlen(dest) >= dest_s)  return 0;
	dest[strlen(dest)] = '\0';
	return 1;
}

stock Table:TableCreate(TableImage:tableimage, TableDB:database, tname[], &TableResult:result)
{
	if(!IsValidTableImage(tableimage))  return INVALID_TABLE;
	if(!strlen(tname))  return INVALID_TABLE;

    if(Table:(tableimage_count+1) == INVALID_TABLE)	return INVALID_TABLE;
    table_count++;

    static string[2048];
    if(!TableCreateBuild(tableimage, tname, database, string))
	{
		return INVALID_TABLE;
	}
	SaveVarTable(table_count, tableimage, database, tname);
	result = TableQuery(database, string, false);
	return Table:table_count;
}

stock TableInsertBuild(TableImage:tableimage, tname[], dest[], dest_s = sizeof(dest), {_, Float, bool}:...)
{
	if(!IsValidTableImage(tableimage))  return 0;
	if(!strlen(tname))  return 0;
	
	static string[256];

	SetGVarTableImageTemp(tableimage);

    InitArgumentMarker(4);
	new len = GetGVarInt(tvar_temp, 0);
	if(len != GetArgumentNum())  return 0;
	
	format(dest, dest_s, "INSERT INTO %s (", tname);

    new curarg = 1;
	new type;
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);
	    GetGVarString(tvar_temp, string, sizeof(string), curarg+1);
	    format(dest, dest_s, "%s%s,", dest, string);
	    if(type == TVAR_TYPE_STRING || type == TVAR_TYPE_ARRAY)
	    {
	        curarg++;
	    }
		curarg += 2;
	}
	dest[strlen(dest)-1] = ')';
	format(dest, dest_s, "%s VALUES (", dest);
	
	curarg = 1;
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);
	    curarg += 2;
	    if(type == TVAR_TYPE_INT)
	    {
	        format(dest, dest_s, "%s%i,", dest, GetArgumentRef(i));
	    }
	    if(type == TVAR_TYPE_FLOAT)
	    {
	        format(dest, dest_s, "%s%f,", dest, GetArgumentRef(i));
	    }
	    if(type == TVAR_TYPE_STRING || type == TVAR_TYPE_ARRAY)
	    {
	        format(dest, dest_s, "%s'%s',", dest, "AAA");
	        curarg++;
	    }
	}
	dest[strlen(dest)-1] = ')';
	if(strlen(dest) >= dest_s)  return 0;
	dest[strlen(dest)] = '\0';
	return 1;
}

stock TableCache:TableInsert(Table:table, cached, {_, Float, bool}:...)
{
	if(!IsValidTable(table))	return INVALID_TABLECACHE;
	
	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);

    static string[2000];
    new string_s = sizeof(string);
    
    InitArgumentMarker(2);
    new len = GetArgumentNum();

    new func_result;//Здесь будет хранится ответ от функции
	//Пишем необязательные параметры
	ArgumentPush();
	//Пишем доп. параметры
	#emit PUSH.S string_s
	#emit PUSH.C string
	#emit PUSH.C tname
	#emit PUSH.S tableimage
	//Пишем количество параметров
	#emit LOAD.S.pri len
	#emit ADD.C 4
	#emit SHL.C.pri 2
	#emit PUSH.pri
	#emit PUSH.C TableInsertBuild
	func_result = AddressCall();

    if(func_result == 0)	return INVALID_TABLECACHE;

    new TableResult:result = TableQuery(database, string, false);
    if(!IsValidTableResult(result)) return INVALID_TABLECACHE;
    TableResultClear(result);
    
	format(string, sizeof(string), "SELECT * FROM %s ORDER BY id DESC", tname);
	result = TableQuery(database, string, false);
	if(!IsValidTableResult(result)) return INVALID_TABLECACHE;
	TableGetFieldAssoc(result, "id", string, sizeof(string));
	TableResultClear(result);

    //Если не нужно кэшировать, тогда возвращаем ид
    if(!cached)
    {
        return TableCache:strval(string);
    }
	new TableCache:cache = CreateTableCache();
    if(!IsValidTableCache(cache))   return INVALID_TABLECACHE;
    if(!LinkTableCache(cache, table, string))   return INVALID_TABLECACHE;
	return cache;
}

stock TableDelete(Table:table, &TableResult:result)
{
	if(!IsValidTable(table))    return 0;
	static string[256];
	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);
	format(string, sizeof(string), "DROP TABLE IF EXISTS %s", tname);
    result = TableQuery(database, string, false);
    DeleteGVarTemp(4);
	return 1;
}

stock TableCacheUpdate(TableCache:cache, &TableResult:result, {_, Float, bool}:...)
{
	if(!IsValidTableCache(cache))   return 0;
	
	static expr[] = "%s%s='%s',";
	
	static dest[2000];
	new dest_s = sizeof(dest);
	static param_name[256];
	
	static id[256];
	new Table:table;
	LoadVarTableCache(cache, table, id);
	if(!IsValidTable(table))    return 0;
	
	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);
	if(!IsValidTableImage(tableimage))    return 0;
	
	format(dest, sizeof(dest), "UPDATE %s SET ", tname);

    InitArgumentMarker(2);
	new len = GetArgumentNum();
	new type;

	new curarg = 1;
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);
	    GetGVarString(tvar_temp, param_name, sizeof(param_name), curarg+1);
	    switch(type)
	    {
		    case TVAR_TYPE_INT:     format(dest, sizeof(dest), "%s%s=%i,", dest, param_name, GetArgumentRef(i));
		    case TVAR_TYPE_FLOAT:   format(dest, sizeof(dest), "%s%s=%f,", dest, param_name, GetArgumentRef(i));
		    case TVAR_TYPE_STRING:
			{
			    GetArgument(i);
				#emit PUSH.pri
				#emit PUSH.C param_name
				#emit PUSH.C dest
				#emit PUSH.C expr
				#emit PUSH.S dest_s
				#emit PUSH.C dest
				#emit PUSH.C 24
				#emit SYSREQ.C format
				#emit STACK 28
				curarg++;
			}
		}
		curarg += 2;
	}
	dest[strlen(dest)-1] = '\0';

	format(dest, sizeof(dest), "%s WHERE id = '%s'", dest, id);
	result = TableQuery(database, dest, false);
	return 1;
}

stock TableCacheDeleteRow(TableCache:cache, TableResult:result)
{
	if(!IsValidTableCache(cache))   return 0;
	
	static id[256];
	new Table:table;
	LoadVarTableCache(cache, table, id);
	if(!IsValidTable(table))    return 0;

	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);
	if(!IsValidTableImage(tableimage))    return 0;
	
	format(string, sizeof(string), "DELETE FROM %s WHERE id = '%s'", tname, id);
	result = TableQuery(database, dest, false);
	return 1;
}

stock TableCacheLoadTemp(TableCache:cache, temp[])
{
    if(!IsValidTableCache(cache))   return 0;
    
    static string[2000];
    format(string, sizeof(string), "SELECT ");
    
	static id[256];
	new Table:table;
	LoadVarTableCache(cache, table, id);
	if(!IsValidTable(table))    return 0;

	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);
	if(!IsValidTableImage(tableimage))    return 0;
	
	new len;
	SetGVarTableImageTemp(tableimage);
	GetGVarInt(tvar_temp, 0);
	
	new curarg = 1;
	new type;

	new temp2[32];
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);
	    GetGVarString(tvar_temp, temp2, sizeof(temp2), curarg+1);
	    format(string, sizeof(string), "%s%s,", string, temp2);
		if(type == TVAR_TYPE_STRING)
		{
		    curarg++;
		}
		curarg += 2;
	}
	string[strlen(string)-1] = '\0';

	format(string, sizeof(string), "%s FROM %s WHERE id = '%s'", string, tname, id);
	new TableResult:result = TableQuery(database, string, false);
	
	curarg = 1;
	new mark_write = 0;
	new len2;
	for(new i = 0; i < len; i++)
	{
	    type = GetGVarInt(tvar_temp, curarg);

		switch(type)
		{
		    case TVAR_TYPE_INT:		temp[mark_write] = TableGetFieldInt(result, i), mark_write++;
		    case TVAR_TYPE_FLOAT:	temp[mark_write] = _:TableGetFieldFloat(result, i), mark_write++;
		    case TVAR_TYPE_STRING:
		    {
		        len2 = GetGVarInt(tvar_temp, curarg+2);
		        TableGetField(result, i, temp[mark_write], len2);
		        mark_write += len2;
		        curarg++;
		    }
		}
		curarg += 2;
	}
	TableResultClear(result);
	return 1;
}

stock TableCacheSaveTemp(TableCache:cache, temp[], temp_s = sizeof(temp))
{
    if(!IsValidTableCache(cache))   return 0;
    
    static string[2000];
    
	static id[256];
	new Table:table;
	LoadVarTableCache(cache, table, id);
	if(!IsValidTable(table))    return 0;

	static tname[256];
	new TableImage:tableimage, TableDB:database;
	LoadVarTable(table, tableimage, database, tname);
	if(!IsValidTableImage(tableimage))    return 0;
	
	new len;
	SetGVarTableImageTemp(tableimage);
	GetGVarInt(tvar_temp, 0);
	
	new temp2[32];
	
	format(string, sizeof(string), "UPDATE %s SET ", tname);
	
	new curarg = 1;
	new type;
	new mark_read = 0;
	new len2;
	for(new i = 0; i < len; i++)
	{
	    if(mark_read >= temp_s) return 0;
	    type = GetGVarInt(tvar_temp, curarg);
		GetGVarString(tvar_temp, temp2, sizeof(temp2), curarg+1);
		switch(type)
		{
		    case TVAR_TYPE_INT:
		    {
		        format(string, sizeof(string), "%s%s = %i,", string, temp2, temp[mark_read]);
		        mark_read++;
		    }
		    case TVAR_TYPE_FLOAT:
		    {
		        format(string, sizeof(string), "%s%s = %f,", string, temp2, Float:temp[mark_read]);
		        mark_read++;
		    }
		    case TVAR_TYPE_STRING:
		    {
				len2 = GetGVarInt(tvar_temp, curarg+2);
		        format(string, sizeof(string), "%s%s = '%s',", string, temp2, temp[mark_read]);
		        mark_read += len2;
		        curarg++;
		    }
		}
		curarg += 2;
	}
	string[strlen(string)-1] = '\0';
	
	format(string, sizeof(string), "%s WHERE id = '%s'", string, id);
	TableQuery(database, string, true);
	return 1;
}















































































