package main

import (
	"encoding/binary"
	"errors"
	"log"
	"net"
	"runtime"
	"time"

	"github.com/sniperHW/netgo"
)

type Codec struct {
	r         int
	w         int
	buff      []byte
	totalRecv int
}

func (codec *Codec) read(readable netgo.ReadAble, deadline time.Time) (int, error) {
	if err := readable.SetReadDeadline(deadline); err != nil {
		return 0, err
	} else {
		return readable.Read(codec.buff[codec.w:])
	}
}

func (codec *Codec) Recv(readable netgo.ReadAble, deadline time.Time) (pkt []byte, err error) {
	const lenHead int = 4
	for {
		rr := codec.r
		pktLen := 0
		if (codec.w - rr) >= lenHead { //} && uint32(codec.w-rr-lenHead) >= binary.LittleEndian.Uint32(codec.buff[rr:]) {
			pktLen = int(binary.BigEndian.Uint32(codec.buff[rr:]))
			rr += lenHead
		}

		if pktLen > 0 {

			log.Println("pklen", pktLen, codec.w-codec.r)

			if pktLen > (len(codec.buff) - lenHead) {
				err = errors.New("pkt too large")
				return
			}
			if (codec.w - rr) >= pktLen {
				pkt = codec.buff[rr : rr+pktLen]
				rr += pktLen
				codec.r = rr
				if codec.r == codec.w {
					codec.r = 0
					codec.w = 0
				}
				return
			}
		}

		if codec.r > 0 {
			//移动到头部
			copy(codec.buff, codec.buff[codec.r:codec.w])
			codec.w = codec.w - codec.r
			codec.r = 0
		}

		var n int
		n, err = codec.read(readable, deadline)
		codec.totalRecv += n
		log.Println(codec.totalRecv, n, err, deadline)
		if n > 0 {
			codec.w += n
		}
		if nil != err {
			return
		}
	}
}

var request string = `set_pot 50
set_effective_stack 200
set_board Qs,Jh,2h
set_range_ip AA,KK,QQ,JJ,TT,99:0.75,88:0.75,77:0.5,66:0.25,55:0.25,AK,AQs,AQo:0.75,AJs,AJo:0.5,ATs:0.75,A6s:0.25,A5s:0.75,A4s:0.75,A3s:0.5,A2s:0.5,KQs,KQo:0.5,KJs,KTs:0.75,K5s:0.25,K4s:0.25,QJs:0.75,QTs:0.75,Q9s:0.5,JTs:0.75,J9s:0.75,J8s:0.75,T9s:0.75,T8s:0.75,T7s:0.75,98s:0.75,97s:0.75,96s:0.5,87s:0.75,86s:0.5,85s:0.5,76s:0.75,75s:0.5,65s:0.75,64s:0.5,54s:0.75,53s:0.5,43s:0.5
set_range_oop QQ:0.5,JJ:0.75,TT,99,88,77,66,55,44,33,22,AKo:0.25,AQs,AQo:0.75,AJs,AJo:0.75,ATs,ATo:0.75,A9s,A8s,A7s,A6s,A5s,A4s,A3s,A2s,KQ,KJ,KTs,KTo:0.5,K9s,K8s,K7s,K6s,K5s,K4s:0.5,K3s:0.5,K2s:0.5,QJ,QTs,Q9s,Q8s,Q7s,JTs,JTo:0.5,J9s,J8s,T9s,T8s,T7s,98s,97s,96s,87s,86s,76s,75s,65s,64s,54s,53s,43s
set_bet_sizes oop,flop,bet,50
set_bet_sizes oop,flop,raise,60
set_bet_sizes oop,flop,allin
set_bet_sizes ip,flop,bet,50
set_bet_sizes ip,flop,raise,60
set_bet_sizes ip,flop,allin
set_bet_sizes oop,turn,bet,50
set_bet_sizes oop,turn,raise,60
set_bet_sizes oop,turn,allin
set_bet_sizes ip,turn,bet,50
set_bet_sizes ip,turn,raise,60
set_bet_sizes ip,turn,allin
set_bet_sizes oop,river,bet,50
set_bet_sizes oop,river,donk,50
set_bet_sizes oop,river,raise,60,100
set_bet_sizes oop,river,allin
set_bet_sizes ip,river,bet,50
set_bet_sizes ip,river,raise,60,100
set_bet_sizes ip,river,allin
set_allin_threshold 0.67
build_tree
set_thread_num 8
set_accuracy 0.5
set_max_iteration 200
set_print_interval 10
set_use_isomorphism 1
start_solve
set_dump_rounds 2
dump_result output_result.json
`

func AppendBytes(bs []byte, bytes []byte) []byte {
	return append(bs, bytes...)
}

func AppendUint32(bs []byte, u32 uint32) []byte {
	bu := []byte{0, 0, 0, 0}
	binary.BigEndian.PutUint32(bu, u32)
	return AppendBytes(bs, bu)
}

func main() {
	dialer := &net.Dialer{}

	go func() {
		for {
			time.Sleep(time.Second)
			runtime.GC()
		}
	}()

	for {
		if conn, err := dialer.Dial("tcp", "localhost:8081"); nil != err {
			time.Sleep(time.Second)
		} else {
			s := netgo.NewTcpSocket(conn.(*net.TCPConn), &Codec{buff: make([]byte, 1024*1024*50)})
			var req []byte
			req = AppendUint32(req, uint32(len(request)))
			req = AppendBytes(req, []byte(request))
			n, err := s.Send(req)
			log.Println(n, err)
			resp, err := s.Recv()
			log.Println(resp, err)
			log.Printf("%p", s)
			break
		}
	}
}
