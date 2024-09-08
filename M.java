import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class M {

    static ArrayList<Entity> entities = new ArrayList<>();
    static ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(10);

    public static void main(String[] args) {

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            for (Entity entity : entities) {
                try {
                    entity.stop();
                } catch (IOException | InterruptedException ex) {
                    System.err.println("Failed to stop entity: " + ex.getMessage());
                }
            }
        }));

        try {
            for (int i = 0; i < 10; i++) {
                Entity entity = new Entity(1, 1, 1);
                scheduler.scheduleAtFixedRate(entity, 0, 1000L / 1, TimeUnit.MILLISECONDS);
                entities.add(entity);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    static class Entity implements Runnable {

        final int[] self = new int[4];
        final int[] entity = new int[4];

        Process         process;
        OutputStream    out;
        InputStream     in;
        InputStream     err;

        public Entity(int scene, int shape, int state)  throws IOException {
            self[0] = 0;
            self[1] = scene;
            self[2] = shape;
            self[3] = state;
            ProcessBuilder builder = new ProcessBuilder("./main");
            builder.directory(new File(System.getProperty("user.dir")));
            builder.redirectErrorStream(true);
            this.process    = builder.start();
            this.out        = process.getOutputStream();
            this.in         = process.getInputStream();
            this.err        = process.getErrorStream();
        }

        public void stop() throws IOException, InterruptedException {
            out.close();
            in.close();
            err.close();
            process.destroy();
            process.waitFor();
        }

        @Override
        public void run() {

            try {
                out();
                in();
            } catch (IOException e) {
                try {
                    stop();
                } catch (IOException | InterruptedException ex) {
                    throw new RuntimeException(ex);
                }
                throw new RuntimeException(e);
            }

        }

        private void out() throws IOException {

            byte[] send = new byte[]{
                    (byte) (self[0] & 0xFF),
                    (byte) (self[1] & 0xFF),
                    (byte) (self[2] & 0xFF),
                    (byte) (self[3] & 0xFF)
            };

            print("OUT: ", send);
            out.write(send);
            out.flush();

        }

        private void in() throws IOException {

            int bytes = 0;
            byte[] buffer = new byte[4];

            while (bytes < 4) {
                int result = in.read(buffer, bytes, 4 - bytes);
                if (result == -1) {
                    break;
                }
                bytes += result;
            }

            for (int i = 0; i < 4; i++) {
                entity[i] = buffer[i] & 0xFF;
            }

            print("IN:  ", buffer);

        }

    }

    private static void print(String prefix, byte... bytes) {
        StringBuilder sb = new StringBuilder(prefix);
        for (byte b : bytes) {
            String binary = String.format("%8s", Integer.toBinaryString(b & 0xFF)).replace(' ', '0');
            sb.append(binary).append(" (").append(b & 0xFF).append(") ");
        }
        System.out.println(sb.toString().trim());
    }

    private static void print(String prefix, int... ints) {
        StringBuilder sb = new StringBuilder(prefix);
        for (int i : ints) {
            String binary = String.format("%8s", Integer.toBinaryString(i & 0xFF)).replace(' ', '0');
            sb.append(binary).append(" (").append(i & 0xFF).append(") ");
        }
        System.out.println(sb.toString().trim());
    }

}
