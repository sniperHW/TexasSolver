package main

import (
	"encoding/binary"
	"errors"
	"log"
	"net"
	"sync"
	"time"

	"github.com/sniperHW/netgo"
)

type Codec struct {
	r    int
	w    int
	buff []byte
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
		if (codec.w - rr) >= lenHead { //&& uint32(codec.w-rr-lenHead) >= binary.LittleEndian.Uint32(codec.buff[rr:]) {
			pktLen = int(binary.BigEndian.Uint32(codec.buff[rr:]))
			rr += lenHead
		}

		if pktLen > 0 {
			if pktLen > (len(codec.buff) - lenHead) {
				err = errors.New("pkt too large")
				return
			}
			if (codec.w - rr) >= pktLen {
				pkt = codec.buff[codec.r : codec.r+pktLen+lenHead]
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
		if n > 0 {
			codec.w += n
		}
		if nil != err {
			return
		}
	}
}

func main() {
	var mtx sync.Mutex
	avalabileSolver := map[netgo.Socket]struct{}{}
	//监听texasSolver连接
	{
		_, serve, err := netgo.ListenTCP("tcp", "localhost:8080", func(conn *net.TCPConn) {
			log.Println("solver connect")
			go func() {
				texasSolver := netgo.NewTcpSocket(conn)
				mtx.Lock()
				avalabileSolver[texasSolver] = struct{}{}
				mtx.Unlock()
			}()
		})
		log.Println(err)
		go serve()
	}

	getSolver := func() netgo.Socket {
		mtx.Lock()
		defer mtx.Unlock()
		for {
			if len(avalabileSolver) > 0 {
				for k, _ := range avalabileSolver {
					delete(avalabileSolver, k)
					return k
				}
			} else {
				mtx.Unlock()
				time.Sleep(time.Second)
				mtx.Lock()
			}
		}
	}

	//监听客户端连接
	{
		_, serve, _ := netgo.ListenTCP("tcp", "localhost:8081", func(conn *net.TCPConn) {
			log.Println("client connect")
			go func() {
				cli := netgo.NewTcpSocket(conn, &Codec{buff: make([]byte, 1024*1024*10)})
				defer cli.Close()

				request, err := cli.Recv()
				if err != nil {
					return
				}
				solver := getSolver()
				defer solver.Close()
				if _, err = solver.Send(request); err != nil {
					return
				}

				totalRecv := 0
				totalSend := 0

				for {
					if response, err := solver.Recv(); err != nil {
						log.Println("1", err)
						return
						
					} else {
						totalRecv += len(response)
						log.Println("send resp ", len(response), totalRecv)
						if n, err := cli.Send(response); err != nil {
							log.Println("2", err)
							return
						} else {
							totalSend += n
							log.Println("totalSend", totalSend)
						}
					}
				}
			}()
		})
		serve()
	}
}
