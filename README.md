# DnsResolver
C# async wrapper for DnsQueryEx using C++ to call native method.

## Example
```csharp
// LONG is the same size as int in Windows
[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate void CallbackDelegate(int status, string ipAddress, uint ttl);

public enum DnsRecordTypes: ushort
{
    DNS_TYPE_A = 0x0001,
    DNS_TYPE_AAAA = 0x001c,
}

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

// You can get the entry point by using VS Command Prompt
// > dumpbin /exports DnsResolver.dll
[DllImport("DnsNameResolver.dll", EntryPoint = @"ResolveDnsName", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
internal static extern void ResolveDns(string hostname, DnsRecordTypes type, CallbackDelegate callback);
```
