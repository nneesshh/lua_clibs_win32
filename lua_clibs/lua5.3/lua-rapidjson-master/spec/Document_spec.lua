--luacheck: ignore describe it
local function fileContents(filename)
	local f = io.open(filename, 'rb')
	if not f then return nil end
	local c = f:read('*a')
	f:close()
	return c
end

describe('rapidjson.Document', function()
  local rapidjson = require('rapidjson')
	local doc
	before_each(function()
		doc = rapidjson.Document()
	end)
  describe('rapidjson.Document()', function()
		describe('creates a Document object', function()
			it('with no args', function()
				assert.are.equals('userdata', type(doc))
			end)
			it('with a json string', function()
				local d = rapidjson.Document('{"a": ["b", "c"]}')
				assert.are.equals('userdata', type(d))
			end)
			it('with a lua table', function()
				local d = rapidjson.Document({a= {"b", "c"}})
				assert.are.equals('userdata', type(d))
			end)
		end)
		describe('raise error if', function()
			it('arg is nil', function()
				assert.has.error(function()
					rapidjson.Document(nil)
				end)
			end)
			it('arg is boolean', function()
				assert.has.error(function()
					rapidjson.Document(true)
				end)
				assert.has.error(function()
					rapidjson.Document(false)
				end)
			end)
			it('arg is number', function()
				assert.has.error(function()
					rapidjson.Document(10)
				end)
			end)
			it('arg is userdata', function()
				assert.has.error(function()
					rapidjson.Document(io.output)
				end)
			end)
			it('arg is function', function()
				assert.has.error(function()
					rapidjson.Document(function() end)
				end)
			end)
		end)
	end)
	describe(':parse()', function()
		it('parses json string into current Document.', function()
			assert.are.equal(true, doc:parse('[]'))
		end)
		it('return nil plus a error message when json string has errors', function()
			local r, m = doc:parse('[')
			assert.is_nil(r)
			assert.are.equal('string', type(m))
		end)
	end)

	describe(':parseFile()', function()
		it('parses a json file into current Document.', function()
			assert.are.equal(true, doc:parseFile('rapidjson/bin/jsonchecker/pass1.json'))
		end)
		it('returns nil plus a error message when its not a valid json file', function()
			local r, m = doc:parseFile('rapidjson/bin/jsonchecker/fail3.json')
			assert.is_nil(r)
			assert.are.equal('string', type(m))
		end)
	end)

	describe(':get() gets Lua repenstion of JSON values by JSON Pointer', function()
		before_each(function()
			doc:parse('{"a": ["apple", "air"], "/":10, "~": 0.5, " ": "ws"}')
		end)
		it('"" returns entire docuement', function()
			assert.are.same({a = {'apple', 'air'}, ['/'] = 10, ['~'] = 0.5, [' '] = 'ws'}, doc:get(''))
		end)
		it('"/a" retusn root object value of key "a"', function()
			assert.are.same({'apple', 'air'}, doc:get('/a'))
		end)
		it('"/a/0" returns first value in array value of "a"', function()
			assert.are.equals('apple', doc:get('/a/0'))
		end)
		it('returns nil when the value does not exists', function()
			assert.is_nil(doc:get('/d'))
			assert.is_nil(doc:get('/a/3'))
		end)
		it('support pass 2nd arg as default value', function()
			assert.are.equals('disk', doc:get('/d', 'disk'))
			assert.are.equals('ace', doc:get('/a/3', 'ace'))
		end)
		it('supports special escape characters', function()
			assert.are.equals(10, doc:get('/~1')) -- ~1 is /
			assert.are.equals(0.5, doc:get('/~0')) -- ~0 is ~
		end)
		it('supports empty space in Pointer', function()
			assert.are.equals('ws', doc:get('/ '))
		end)
	end)

	describe(':set() sets Lua value to JSON Document with JSON Pointer', function()
		it('sets boolean values', function()
			assert.is_nil(doc:get('/a'))
			doc:set('/a', true)
			assert.are.same(true, doc:get('/a'))
		end)
		it('sets number values', function()
			assert.is_nil(doc:get('/a'))
			doc:set('/a', 1)
			assert.are.same(1, doc:get('/a'))
		end)
		it('sets string values', function()
			assert.is_nil(doc:get('/a'))
			doc:set('/a', 'apple')
			assert.are.same('apple', doc:get('/a'))
		end)
		it('sets array values', function()
			local v = {'apple', 'air'}
			assert.is_nil(doc:get('/a'))
			doc:set('/a', v)
			assert.are.same(v, doc:get('/a'))
		end)
		it('sets object values', function()
			local v = {a = 'apple', b = 'bike'}
			assert.is_nil(doc:get('/a'))
			doc:set('/a', v)
			assert.are.same(v, doc:get('/a'))
		end)
		it('create parent nodes if needed', function()
			assert.is_nil(doc:get('/a'))
			doc:set('/a/b/c', true)
			assert.are.same( {b={c=true}}, doc:get('/a'))
		end)
	end)

	describe(':stringify()', function()
		it('serializes docuement to string', function()
			doc:parse('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}')
			assert.are.equals('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}', doc:stringify())
		end)
		it('support pretty pring', function()
			doc:parse('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}')
			assert.are.equals(
[[{
    "a": [
        "apple",
        "air"
    ],
    "/": 10,
    "~": 0.5,
    " ": "ws"
}]], doc:stringify({pretty=true}))
		end)
	end)
	describe(':save()', function()
		local filename = '.temp_json_file.json'
		after_each(function()
			os.remove(filename)
		end)
		it('saves docuement to file', function()
			doc:parse('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}')
			doc:save(filename)
			assert.are.equals('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}', fileContents(filename))
		end)
		it('support pretty pring', function()
			doc:parse('{"a":["apple","air"],"/":10,"~":0.5," ":"ws"}')
			doc:save(filename, {pretty=true})
			assert.are.equals(
[[{
    "a": [
        "apple",
        "air"
    ],
    "/": 10,
    "~": 0.5,
    " ": "ws"
}]], fileContents(filename))
		end)
	end)
end)
