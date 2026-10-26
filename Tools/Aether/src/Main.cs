using System;

namespace Aether;

internal static class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("====================================");
        Console.WriteLine(" Aether(ion) Build Tool v0.1");
        Console.WriteLine("====================================");

        if(args.Length == 0)
        {
            Console.WriteLine("Usage: aether <target> <platform> <debug:release>");
            return;
        }
    }
}