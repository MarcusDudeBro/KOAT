import { createRouter, createWebHistory } from 'vue-router';
import HomePage from './components/HomePage.vue'
import FindPage from './components/FindPage.vue'
import TrackPage from './components/TrackPage.vue';

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'Home',
      component: HomePage
    },
    {
      path: '/find',
      name: 'Find',
      component: FindPage
    },
    {
      path: '/track',
      name: 'Track',
      component: TrackPage
    }
  ]
});

export default router;
