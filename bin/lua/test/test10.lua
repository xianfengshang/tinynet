local public_key = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEApphGdl0aMWG/L4pYsjIvrn7RsGLLCgYzRD01txz3FZXUT/gipFYf5jKkFQVuUPaEif0JWTmfGVWCBAMbFo+i+F0uBa64T9DuzncUptnhzirLWF9zo0gOcJeAbmioPGhe0MQqJ6s/GrT5qIqjoRDr4z7YxssxxYQKx9bjG3vGjqwEa/xN2o/DPR5jE3hfq4REk/Wg2x+b+WqDrZv/hkmqOWPXX0xJUglrzn4OuVp16v0wUwPS21n/NkGpaQGcoEpbCHH3fwso0RHgil4DkaSNX0utrpcr5LR4ODuyedjtOTecoc9VvyWRpVqNqKE5yutHkHPWYinpbXRmC9ljbfyR+wIDAQAB"
local private_key = "MIIEpAIBAAKCAQEApphGdl0aMWG/L4pYsjIvrn7RsGLLCgYzRD01txz3FZXUT/gipFYf5jKkFQVuUPaEif0JWTmfGVWCBAMbFo+i+F0uBa64T9DuzncUptnhzirLWF9zo0gOcJeAbmioPGhe0MQqJ6s/GrT5qIqjoRDr4z7YxssxxYQKx9bjG3vGjqwEa/xN2o/DPR5jE3hfq4REk/Wg2x+b+WqDrZv/hkmqOWPXX0xJUglrzn4OuVp16v0wUwPS21n/NkGpaQGcoEpbCHH3fwso0RHgil4DkaSNX0utrpcr5LR4ODuyedjtOTecoc9VvyWRpVqNqKE5yutHkHPWYinpbXRmC9ljbfyR+wIDAQABAoIBAEtqe9txYj4gpc/7rRNeLL+toXdcAdZT4DlwDDUM473dyfM5vVTvuBkZq7Xoc3wrXOyqo+TEdQWe8/ClB0o7U+djJ0ZjRgS77J+CwNKXp+UQ8mTbMRolsIO/0eApeJ+AEAwqAXtbxoFsk+h8en6bjm2dMb0slyd3EvpMiRsT+Vr0QDy9hJ+EbAE+MwUcoGIBgDLaAElYLICaIzwE/5vzplcBsJdqX/F2nVm3FnWK50CGv72hbCYsOm/Vb8DZ3jiJCzB9RtYRNYGbBsMwkvNrxgY8eqG1aOXHo5W+5ny6G8ZkwDVrqAiBzvYu3SbnK/jGnJam7wcPij8EeZhJNcpPA4ECgYEA5T9Wibx425G49XoMRNsafbReiA11kfFPenq/dbrf0J4iu1Xyc0fiB0BYsSr91HD6LTXoictv7qrfhWLP92h7wB10qcGc4+hEV8TzybKoV1cn26+g2b8fp5LQnopZUz1/eOfynxT7aZ+WNh8jG6YlhMxoW1fMQuDKytNcxWRS5+sCgYEAugk4PL1VhRRWlZLvu7OlS/YcF39YU1XhXL3aQ3G27/vlnOg5o4uQbt9bT7XEPSVCahqlXaMD6UzO5umRFpR8qw7P9NWOecM2Cx3Ho/ibR5Uur2SKVaPMlHla9Qws7tohoWCkKjh0k28pM46QSFJhIw12iFif1vPOJxrHiDptCjECgYEA4XLXpR8P8EcwEWGUnUaol8UBTmWGIR+inP4IOjysjVpnJ7rPc+XWeHEkiB+SfxEYR8wDQzgpfRrYNTJG+bXsNbQCHkwFAfFS/Mn5QnuLqFFTJm5jDqEx/TeaKMBANmd9bCcXaLFWyroiHFmkyjUsIpxvaJ1AiOjPJoaUbp9KFT8CgYBbNhIAlcPMT8Bz7Wrk2Jt+ttfnHWSs1zp7Qgo1bxeGajaVy7YW3WKfOZuHZVMlt5LseswBMN/GDwaSaIneTEcjh0umet40DOX+ZUdwuX9IhXgyPlUkz/6J/UX/R0zwfrpmaP+UQ8HEV4gX4xoSm3FMwlkPWIRl23uD6LlTrXGwwQKBgQCxwh7NSorfj3slSWs/tZ3IKtfbn1YaSubokx4wXTGTOPGhGNLmuOM1uf+zKWXbE+Ih2Bqb2Dy0gBYVOcOHC/OwUCt0ZsgygPvbJq6rqkL3V8QM1wypi26nvdFoRoS5r4u+jbDkXSoqRxA5mfRGbBJZt4GAS44X/35Y5O9zQJRpuA=="


public_key = string.format("-----BEGIN PUBLIC KEY-----\n%s\n-----END PUBLIC KEY-----",
                                                public_key)
private_key = string.format("-----BEGIN RSA PRIVATE KEY-----\n%s\n-----END RSA PRIVATE KEY-----",
                                                private_key)

local data = "Hello World"

local sign = openssl_sign(data, private_key, "sha1")

local res = openssl_verify(data, sign, public_key, "sha1")

log.warning("res = %s", tostring(res))