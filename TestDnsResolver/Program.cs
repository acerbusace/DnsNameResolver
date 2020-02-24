using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace TestDnsResolver
{
    class Program
    {
        // LONG is the same size as int in Windows
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void CallbackDelegate(int status, string ipAddress, uint ttl);

        static async Task Main(string[] args)
        {
            var result = await GetHostAddressAsync("www.bing.com").ConfigureAwait(false);
            Console.WriteLine($"Result: {result}");
            Console.ReadKey();
        }

        static async Task<Tuple<string, uint>> GetHostAddressAsync(string hostname, DnsRecordTypes type = DnsRecordTypes.DNS_TYPE_A)
        {
            var taskWrapper = new TaskCompletionSource<Tuple<string, uint>>();
            var callback = new CallbackDelegate(
                (status, ipAddress, ttl) =>
                {
                    if (status != 0)
                    {
                        taskWrapper.SetException(new Exception($"DNS query for {hostname} failed. ErrorCode: {status}"));
                        return;
                    }

                    Console.WriteLine($"Status: {status} | IP: {ipAddress} | TTL: {ttl}");
                    taskWrapper.SetResult(Tuple.Create(ipAddress, ttl));
                });

            ResolveDns(hostname, type, callback);
            return await taskWrapper.Task;
        }

        public enum DnsRecordTypes: ushort
        {
            DNS_TYPE_A = 0x0001,
            DNS_TYPE_AAAA = 0x001c,
        }

        // You can get the entry point by using VS Command Prompt
        // > dumpbin /exports DnsResolver.dll
        [DllImport("DnsResolver.dll", EntryPoint = @"_ResolveDns@12", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
        internal static extern void ResolveDns(string hostname, DnsRecordTypes type, CallbackDelegate callback);
    }
}
