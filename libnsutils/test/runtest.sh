#!/bin/sh 
TEST_PATH=$1

b64enctst()
{
    ENC=$(echo -n "${1}" | ${TEST_PATH}/test_base64 )
    if [ "${ENC}" != "${2}" ];then
        echo "Base64 encode error '${ENC}' != '${2}'"
	exit 2
    fi
}

b64dectst()
{
    DEC=$(echo -n "$1" | ${TEST_PATH}/test_base64 -d )
    if [ "${DEC}" != "$2" ];then
        echo "Base64 decode error '${DEC}' != '$2'"
	exit 3
    fi
}

b64urlenctst()
{
    ENC=$(echo -n "${1}" | ${TEST_PATH}/test_base64 -u )
    if [ "${ENC}" != "${2}" ];then
        echo "Base64 url encode error '${ENC}' != '${2}'"
	exit 2
    fi
}

b64urldectst()
{
    DEC=$(echo -n "$1" | ${TEST_PATH}/test_base64 -d -u )
    if [ "${DEC}" != "$2" ];then
        echo "Base64 url decode error '${DEC}' != '$2'"
	exit 3
    fi
}

b64enctst      'f' 'Zg=='
b64enctst     'fo' 'Zm8='
b64enctst    'foo' 'Zm9v'
b64enctst    '  >' 'ICA+'
b64enctst    '  ?' 'ICA/'
b64enctst   'foob' 'Zm9vYg=='
b64enctst  'fooba' 'Zm9vYmE='
b64enctst 'foobar' 'Zm9vYmFy'

b64dectst     'Zg==' 'f'
b64dectst     'Zm8=' 'fo'
b64dectst     'Zm9v' 'foo'
b64dectst     'ICA+' '  >'
b64dectst     'ICA/' '  ?'
b64dectst 'Zm9vYg==' 'foob'
b64dectst 'Zm9vYmE=' 'fooba'
b64dectst 'Zm9vYmFy' 'foobar'

b64urlenctst      'f' 'Zg=='
b64urlenctst     'fo' 'Zm8='
b64urlenctst    'foo' 'Zm9v'
b64urlenctst    '  >' 'ICA-'
b64urlenctst    '  ?' 'ICA_'
b64urlenctst   'foob' 'Zm9vYg=='
b64urlenctst  'fooba' 'Zm9vYmE='
b64urlenctst 'foobar' 'Zm9vYmFy'

b64urldectst     'Zg==' 'f'
b64urldectst     'Zm8=' 'fo'
b64urldectst     'Zm9v' 'foo'
b64urldectst     'ICA-' '  >'
b64urldectst     'ICA_' '  ?'
b64urldectst 'Zm9vYg==' 'foob'
b64urldectst 'Zm9vYmE=' 'fooba'
b64urldectst 'Zm9vYmFy' 'foobar'
