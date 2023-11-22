pb.clear()

pb.mapping("", "lua/test/")

pb.import("test.proto")

local alice = {
	name = "Alice",
	id = 12345,
	phones = {
		{ number = "1301234567", type = 0 },
		{ number = "87654321", type = 2 },
		{ number = "13912345678", type = 0 },
	},
	email = "username@domain.com",
    age = 20,
    country = "USA",
}

local bob = {
	name = "Bob",
	id = 22345,
	phones = {
		{ number = "1301234567", type = 0},
		{ number = "87654321", type = 2 },
		{ number = "13912345678", type = 0 },
	},
	email = "username@domain.com",
    age = 21,
    country = "UK",
}

local address_book = {people={alice, bob}}

local awesome_message = {
    --required fields
    required_double_field = 3.1415926,
    required_float_field = 3.14,
    required_int64_field = 1111111111,
    required_uint64_field = 222222222,
    required_int32_field = 123,
    required_fixed64_field = 333333333,
    required_fixed32_field = 444444444,
    required_bool_field = true,
    required_string_field = "hello",
    required_message_field = address_book,
    required_bytes_field = "���",
    required_uint32_field = 456,
    required_enum_field = 1,
    required_sfixed32_field = 789,
    required_sfixed64_field = 5555555555,
    required_sint32_field = 1000,
    required_sint64_field = 66666666,

    --optional fields
    optional_int32_field = 123,
    optional_uint32_field = 456,
    optional_string_field = "hello",
    optional_bytes_field = "���",

    --repeated fields
    repeated_int32_field = {1, 2, 3, 4, 5, 6, 7, 8, 9},
    repeated_string_field = {"a", "b", "c", "d", "e", "f", "g"},
    repeated_message_field = {address_book}

}

local opts = { encode_as_bytes = true, bytes_as_string = false}
local begin = high_resolution_time()
for i = 1, 10000 do
    local bin = pb.encode("Testing.AwesomeMessage", awesome_message, opts)
    local msg = pb.decode("Testing.AwesomeMessage", bin, opts)
end
local elapse = high_resolution_time() - begin
log.warning("elapse = %.6f ms", elapse * 1000)

--Note: Encoding and decoding Extension fields are really slow 
